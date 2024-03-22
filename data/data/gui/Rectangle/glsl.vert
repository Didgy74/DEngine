#version 450 core
#include "Uniforms.glsl"

vec2 positions[6] = { 
	{ 0.0, 0.0 },
	{ 0.0, 1.0 },
	{ 1.0, 0.0 },
	{ 1.0, 0.0 },
	{ 0.0, 1.0 },
	{ 1.0, 1.0 } };

layout(location = 0) out vec2 trianglePos;
layout(location = 1) out vec2 rectExtentOut;
layout(location = 2) out vec2 rectOffsetOut;
layout(location = 3) out vec2 out_startPoint;
layout(location = 4) out vec2 out_endPoint;

vec2 TransformPoint(vec2 p, vec2 flippedOffset, vec2 flippedExtent, int orientation) {
	p *= flippedExtent;

	// If rotated, we need to position it differently
	if (orientation == ENUM_ORIENTATION_0) {
		p += flippedOffset;
	} if (orientation == ENUM_ORIENTATION_90) {
		p.x += flippedOffset.x;
		p.y += 1 - flippedExtent.y - flippedOffset.y;
	} else if (orientation == ENUM_ORIENTATION_180) {
		p.x += 1 - flippedExtent.x - flippedOffset.x;
		p.y += 1 - flippedExtent.y - flippedOffset.y;
	} else if (orientation == ENUM_ORIENTATION_270) {
		p.x += 1 - flippedExtent.x - flippedOffset.x;
		p.y += flippedOffset.y;
	}

	return p;
}

void main()
{
	int orientation = perWindowUniform.orientation;
	mat2 orientMat = OrientationToMat2(orientation);

	vec2 resolution = perWindowUniform.resolution;
	vec2 flippedResolution = resolution;
	flippedResolution = FlipForOrientation(flippedResolution);

	vec2 rectOffset = pushConstData.rectOffset;
	//rectOffset = vec2(0, 0);
	//rectOffset = vec2(50, 100) / flippedResolution;
	//rectOffset = ReorientPoint(rectOffset);
	vec2 flippedRectOffset = rectOffset;
	flippedRectOffset = FlipForOrientation(flippedRectOffset);

	vec2 rectExtent = pushConstData.rectExtent;
	//rectExtent = vec2(0.25, 0.5);
	//rectExtent = vec2(1000, 600) / flippedResolution;
	vec2 flippedRectExtent = rectExtent;
	flippedRectExtent = FlipForOrientation(flippedRectExtent);

	vec2 vtxPos = positions[gl_VertexIndex];

	vec2 outPos = vtxPos;

	out_startPoint = TransformPoint(positions[0], flippedRectOffset, flippedRectExtent, orientation);
	out_endPoint = TransformPoint(positions[5], flippedRectOffset, flippedRectExtent, orientation);
	outPos = TransformPoint(outPos, flippedRectOffset, flippedRectExtent, orientation);
	outPos = outPos * 2 - 1;

	rectExtentOut = rectExtent;
	rectOffsetOut = rectOffset;
	trianglePos = vtxPos;
	//outPos = vtxPos;
	gl_Position = vec4(outPos, 0.5, 1);
}

/*
//outPos *= rectExtent * 2;
outPos *= flippedRectExtent;

// If rotated, we need to position it differently
if (orientation == ENUM_ORIENTATION_0) {
    outPos += rectOffset;
} if (orientation == ENUM_ORIENTATION_90) {
    outPos.x += flippedRectOffset.x;
    outPos.y += 1 - flippedRectExtent.y - flippedRectOffset.y;
} else if (orientation == ENUM_ORIENTATION_180) {
    outPos.x += 1 - flippedRectExtent.x - flippedRectOffset.x;
    outPos.y += 1 - flippedRectExtent.y - flippedRectOffset.y;
} else if (orientation == ENUM_ORIENTATION_270) {
    outPos.x += 1 - flippedRectExtent.x - flippedRectOffset.x;
    outPos.y += flippedRectOffset.y;
}
*/