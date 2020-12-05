#version 450 core

layout(set = 2, binding = 0) uniform sampler2D bleh;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(bleh, uv);
}
