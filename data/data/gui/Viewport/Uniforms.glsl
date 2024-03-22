#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "GlobalGuiUniform.glsl"

layout(push_constant) uniform PushConstData {
	layout(offset = 0) vec2 rectOffset;
	layout(offset = 8) vec2 rectExtent;
} pushConstData;

#endif // UNIFORMS_H