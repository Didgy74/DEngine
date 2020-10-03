#version 450 core

layout(location = 0) in vec2 fragUv;

layout(set = 0, binding = 0) uniform sampler2D myTexture;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(myTexture, fragUv);
}