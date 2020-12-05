#version 450 core

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Color
{
	layout(offset = 32) vec4 color;
} color;

void main()
{
	outColor = color.color;
}