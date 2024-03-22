#version 450 core
#include "Uniforms.glsl"


vec2 positions[6] = { 
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 0 },
	{ 0, 1 },
	{ 1, 1 } };

layout(location = 0) out vec2 fragUv;

void main()
{
	vec2 dstPos;
	for(int i = 0; i < 2; i++) {
		// Apply rect
		dstPos[i] = positions[gl_VertexIndex][i] * pushConstData.rectExtent[i] + pushConstData.rectOffset[i];
		// Convert to NDC
		dstPos[i] = dstPos[i] * 2 - 1;
	}
	
	mat2 orientMat = WindowOrientationMat();
		
	gl_Position = vec4(dstPos * orientMat, 0, 1);
	fragUv = positions[gl_VertexIndex];
}