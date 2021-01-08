#version 450 core

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstantData
{
	layout(offset = 64) vec4 color;
} pushConstantData;

void main()
{
	outColor = pushConstantData.color;
}