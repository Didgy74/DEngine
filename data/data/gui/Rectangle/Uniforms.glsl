#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "GlobalGuiUniform.glsl"

layout(push_constant) uniform PushConstData {
	layout(offset = 0) vec2 rectOffset;
	layout(offset = 8) vec2 rectExtent;
	layout(offset = 16) vec4 color;
	// Each corner radius. Index zero is top-left and increases counter-clockwise.
	// Each value is relative to what the DrawCmd regards as height.
	layout(offset = 32) vec4 radius;
} pushConstData;

vec4 CorrectedRadius() {
	vec4 correctedRadius = pushConstData.radius;
	int orientation = perWindowUniform.orientation;
	for (int i = 0; i < 4; i++) {
		correctedRadius[i] = pushConstData.radius[(i - orientation + 4) % 4];
	}

	vec2 resolution = perWindowUniform.resolution;
	if (orientation == ENUM_ORIENTATION_90 || orientation == ENUM_ORIENTATION_270) {
		correctedRadius = correctedRadius * resolution.x / resolution.y;
	}

	return correctedRadius;
}

#endif // UNIFORMS_H