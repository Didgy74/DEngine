#language slang latest

module textured_quad;

import Gfx.CameraUniform;
import Gfx.ObjectUniform;

[[vk::binding(0, 2)]]
Sampler2D uniform_texture;

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

	float2 GetInputUv() {
        static const float2 in_uvs[4] = {
            { 0.0, 0.0 },
            { 0.0, 1.0 },
            { 1.0, 0.0 },
            { 1.0, 1.0 }, };
        return in_uvs[vertexId];
    }
};

// Output of the vertex shader, and input to the fragment shader.
struct SharedData {
    float2 fragUv; 
}

struct VSOutput {
    SharedData sharedData : SharedData;
    float4 sv_position : SV_Position;
}

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
    var output = VSOutput();
	output.sharedData.fragUv = input.GetInputUv();
	output.sv_position = Gfx.Object.TransformPos(input.GetInputPos());
    return output;
}

[shader("fragment")]
float4 fsMain(SharedData input) : SV_Target
{
    return uniform_texture.Sample(input.fragUv);
}
