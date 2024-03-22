#version 450 core
#include "Uniforms.glsl"

layout(location = 0) in vec2 fragUv;

layout(set = 1, binding = 0) uniform sampler2D fontGlyph;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 color = pushConstData.color;

	float alpha = texture(fontGlyph, fragUv).r;
	
	outColor = vec4(color.xyz, alpha);
}