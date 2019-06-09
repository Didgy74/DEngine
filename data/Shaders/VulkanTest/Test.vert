#version 450 core

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 model;
} cameraData;

layout(set = 0, binding = 1) uniform ObjectData
{
	mat4 model;
} objectData;

layout(location = 0) out FragData
{
	vec3 color;
} fragData;

layout(location = 0) in vec3 vtxPosition;
layout(location = 1) in vec2 vtxUV;
layout(location = 2) in vec3 vtxNormal;

void main()
{
	gl_Position =  cameraData.model * objectData.model * vec4(vtxPosition, 1.0);
	gl_Position.y = -gl_Position.y;
	
	fragData.color = vec3(vtxUV, 0.5);
}
