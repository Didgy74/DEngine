#language slang latest

import Gfx.CameraUniform;
import Gfx.ObjectUniform;
import Gui;
import Gui.WindowUniform;

struct PushConstData {
    // The pixel coordinates relate to the rectangle we are rendering.
    // NOT the source rectangle shape. The source shape will be inferred
    // from the falloff.
    int2 rectPosPx;
    int2 rectExtentPx;
    int4 radiusPx;
    float falloffPx;
    float alpha;

    float AspectRatio() { return (float)rectExtentPx.x / (float)rectExtentPx.y; }
}

[[vk::push_constant]]
ConstantBuffer<PushConstData> pushConstData;

float RectAspectRatio() { return pushConstData.AspectRatio(); }

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
    // We want an alpha mask where 1 is covering the inner rect, and fading outwards and reaches 0 at the edge of our 
    // rendered rect.

    let fragUvNorm = input.fragUv;

    // Assume the SDF rect is centered in our rect being rendered, the p param needs to be relative to that center.
    // fragUvNorm is in the square coordinate space [0, 1], but the rect we are rendering is not. We adjust to 
    // based on aspect ratio.

    let rectExtentPx = pushConstData.rectExtentPx;
    let falloffPx = pushConstData.falloffPx;
    let falloffNorm = falloffPx / rectExtentPx.y;

    // Establish the rectangle for the SDF, where Y size is considered 1.
    let rectAspect = RectAspectRatio();
    var p = fragUvNorm - 0.5;
    p.x *= rectAspect;
    var halfSize = float2(1) / 2.0;
    halfSize.x *= rectAspect;
    halfSize -= falloffNorm;

    let radiuses = Gui.RectCornerRadiusesF.From(float4(pushConstData.radiusPx) / float(rectExtentPx.y));
    let distance = Gui.Sdf.RoundedRect(
        p,
        halfSize,
        radiuses);

    // Linear falloff with 1 at the source shape, and 0 at the edgo of our rect.
    var alpha = distance / falloffNorm;

    // So far, because the SDF distance is increasing as we move away from the shape,
    // we want to invert it so we can use it as an alpha mask.
    alpha = -alpha + 1;
    alpha = saturate(alpha);
    alpha = smoothstep(0, 1, alpha);
    alpha *= pushConstData.alpha;
    return float4(float3(0, 0, 0), alpha);
}
