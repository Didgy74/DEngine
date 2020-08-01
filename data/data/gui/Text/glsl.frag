#version 450 core

layout(location = 0) in vec2 fragUv;

layout(set = 0, binding = 0) uniform sampler2D fontGlyph;

layout(push_constant) uniform ColorData
{
	layout(offset = 32) vec4 color;
} color;

layout(location = 0) out vec4 outColor;

void main()
{
	float alpha = texture(fontGlyph, fragUv).r;
	
	outColor = vec4(color.color.r, color.color.g, color.color.b, alpha);
}