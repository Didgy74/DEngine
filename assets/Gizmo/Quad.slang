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

struct VSInput {
    uint vertexId : SV_VulkanVertexID;

    float3 GetInputPos() {
        static const float3 in_positions[4] = {
            { -0.5, 0.5, 0.0 },
            { -0.5, -0.5, 0.0 },
            { 0.5, 0.5, 0.0 },
            { 0.5, -0.5, 0.0 }, };
        return in_positions[vertexId];
    }
}

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
