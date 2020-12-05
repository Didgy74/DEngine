#version 450 core

layout(location = 0) in vec2 vtxPos;
layout(location = 1) in vec2 vtxUv;

layout(push_constant) uniform PushConstantData
{
	layout(offset = 0) mat2 orientation;
	layout(offset = 16) vec2 rectOffset;
	layout(offset = 24) vec2 rectExtent;
} pushConstantData;

void main()
{
	vec2 dstPos;
	for(int i = 0; i < 2; i++)
	{
		// Apply rect
		dstPos[i] = vtxPos[i] * pushConstantData.rectExtent[i] + pushConstantData.rectOffset[i];
		// Convert to NDC
		dstPos[i] = dstPos[i] * 2 - 1;
	}
		
	gl_Position = vec4(dstPos * pushConstantData.orientation, 0, 1);
}