#version 450 core

layout(location = 0) in FragData
{
	vec3 color;
} fragData;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(fragData.color, 1.0);
}