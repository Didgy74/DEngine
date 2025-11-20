#language slang latest

import Gui;

import Gfx.CameraUniform;
import Gfx.ObjectUniform;
import Gui.WindowUniform;

struct PushConstData {
    int2 rectPosPx;
    int2 rectExtentPx;
    float4 colorA;
    float4 colorB;
    // Angle in the range [0,1] where 0 points down and 1 represents a full turn.
    // Positive values rotate the direction counter-clockwise
    float dirAngle;
}

[[vk::push_constant]]
ConstantBuffer<PushConstData> pushConstData;

struct VSInput {
    uint vertexId : SV_VulkanVertexID;

    float2 GetInputPos() {
        static const float2 in_positions[6] = {
            { 0.0, 0.0 },
            { 0.0, 1.0 },
            { 1.0, 0.0 },
            { 1.0, 0.0 },
            { 0.0, 1.0 },
            { 1.0, 1.0 } };
        return in_positions[vertexId];
    }
};

// Output of the vertex shader, and input to the fragment shader.
struct SharedData {
    float2 fragUv; 
}

struct VSOutput {
    SharedData coarseVertex : SharedData;
    float4 sv_position : SV_Position;
}

[shader("vertex")]
VSOutput vsMain(VSInput input) {
    VSOutput output = {};
    var inputPos = input.GetInputPos();
    output.coarseVertex.fragUv = inputPos;

    // Move from [0,1] to [-0.5 , 0.5]
    var normalizedPos = inputPos;

    let rectExtentNorm = float2(pushConstData.rectExtentPx) / Gui::Window::GetOrientedResolution();
    let rectPosNorm = float2(pushConstData.rectPosPx) / Gui::Window::GetOrientedResolution();
    normalizedPos *= rectExtentNorm;
    normalizedPos += rectPosNorm;

    normalizedPos.xy -= 0.5;
    normalizedPos.y = -normalizedPos.y;

    output.sv_position = Gfx.Object.TransformPos(float3(normalizedPos, 0));

    let orientMat = Gui::Window::GetOrientationMat();
    output.sv_position.xy = mul(orientMat, output.sv_position.xy);

    return output;
}

[shader("fragment")]
float4 fsMain(SharedData input) : SV_Target {
    let fragUvNorm = input.fragUv;
    let dirAngle = pushConstData.dirAngle;
    let colorA = pushConstData.colorA;
    let colorB = pushConstData.colorB;

    let radians = dirAngle * 2 * float.getPi();
    let dir = float2(sin(radians), cos(radians));
    let proj = dot(fragUvNorm, dir);
    let minProj = (dir.x < 0.0 ? dir.x : 0.0) + (dir.y < 0.0 ? dir.y : 0.0);
    let maxProj = (dir.x > 0.0 ? dir.x : 0.0) + (dir.y > 0.0 ? dir.y : 0.0);
    let range = maxProj - minProj;

    // Normalize to [0,1] and guard against degenerate direction
    var t = (range > 1e-5) ? (proj - minProj) / range : 0.0;
    t = saturate(t);
    t = smoothstep(0, 1, t);

    return lerp(colorA, colorB, t);
}
