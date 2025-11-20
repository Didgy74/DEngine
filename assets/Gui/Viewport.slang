#language slang latest

import Gfx.CameraUniform;
import Gfx.ObjectUniform;
import Gui.WindowUniform;

[[vk::binding(0, 3)]]
Sampler2D uniform_viewportImg;

struct PushConstData {
    int2 rectPosPx;
    int2 rectExtentPx;
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

public float4 TransformRectPointToFrag(int2 rectExtentPx, int2 rectPosPx, float2 inputPos) {
    let rectExtentNorm = float2(rectExtentPx) / Gui::Window::GetOrientedResolution();
    let rectPosNorm = float2(rectPosPx) / Gui::Window::GetOrientedResolution();

    var outPos = inputPos;

    outPos *= rectExtentNorm;
    outPos += rectPosNorm;
    // Normalize to screenspace
    outPos *= 2;
    outPos -= 1;
    // ?
    let orientMat = Gui::Window::GetOrientationMat();
    return float4(mul(orientMat, outPos), 0, 1);
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
    return uniform_viewportImg.Sample(input.fragUv);
}
