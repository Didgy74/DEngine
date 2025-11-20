#language slang latest

module textured_quad;

import Gfx.CameraUniform;
import Gfx.Helpers;

struct PushConstData {
    column_major float4x4 objectMatrix;
	float4 color;
}

[[vk::push_constant]]
ConstantBuffer<PushConstData> pushConstData;

// Vertex input struct
struct VSInput {
    [[vk::location(0)]]
    float3 position;

    float3 GetInputPos() { return position; }
};

struct VSOutput {
    float4 sv_position : SV_Position;
}

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
    var output = VSOutput();
	output.sv_position = Gfx.TransformPos(
        Gfx.Camera.MakeMvp(pushConstData.objectMatrix),
        input.GetInputPos());
    return output;
}

[shader("fragment")]
float4 fsMain() : SV_Target
{
    return pushConstData.color;
}
