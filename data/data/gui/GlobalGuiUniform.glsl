#ifndef PER_WINDOW_UNIFORM_H
#define PER_WINDOW_UNIFORM_H



layout(set = 0, binding = 0) uniform PerWindowUniform {
	// Strictly follows the Gfx::WindowRotation enum.
	layout(offset = 0) int orientation;
	// Rotation not applied.
	layout(offset = 16) ivec2 resolution;
} perWindowUniform;

const int ENUM_ORIENTATION_0 = 0;
const int ENUM_ORIENTATION_90 = 1;
const int ENUM_ORIENTATION_180 = 2;
const int ENUM_ORIENTATION_270 = 3;

vec2 FlipForOrientation(vec2 param) {
	int orient = perWindowUniform.orientation;
	if (orient == ENUM_ORIENTATION_90 || orient == ENUM_ORIENTATION_270)
		return vec2(param.y, param.x);
	else
		return param;
}

mat2 OrientationToMat2(int orient) {
	switch(orient) {
		case ENUM_ORIENTATION_0: return mat2(1, 0, 0, 1);
		case ENUM_ORIENTATION_90: return mat2(0, 1, -1, 0);
		case ENUM_ORIENTATION_180: return mat2(-1, 0, 0, -1);
		case ENUM_ORIENTATION_270: return mat2(0, -1, 1, 0);
	}
	return mat2(0);
}

mat2 WindowOrientationMat() {
	return OrientationToMat2(perWindowUniform.orientation);
}

// Given a point p, will return the same point from the
// origin of the current orientation.
vec2 ReorientPoint(vec2 p) {
	int orientation = perWindowUniform.orientation;
	switch (orientation) {
		case ENUM_ORIENTATION_0: return p;
		case ENUM_ORIENTATION_90: return vec2(p.y, 1 - p.x);
		case ENUM_ORIENTATION_180: return vec2(1.0 - p.x, 1.0 - p.y);
		case ENUM_ORIENTATION_270: return vec2(1 - p.y, p.x);
		default: return vec2(0);
	}
}

#endif // PER_WINDOW_UNIFORM_H