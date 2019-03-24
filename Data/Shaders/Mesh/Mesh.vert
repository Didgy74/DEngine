#version 420 core

layout(location = 0) in vec3 vtxPosition;
layout(location = 1) in vec2 vtxUV;
layout(location = 2) in vec3 vtxNormal;

layout(std140, binding = 0) uniform CameraData
{
	vec3 wsPosition;
	mat4 viewProjection;
} cameraData;

uniform mat4 model;

out FragData
{
	vec3 wsPosition;
	vec3 wsNormal;
	vec2 uv;
} fragData;

void main()
{
	gl_Position = cameraData.viewProjection * model * vec4(vtxPosition, 1.0);
	
	fragData.wsPosition = vec3(model * vec4(vtxPosition, 1.0));
	fragData.wsNormal = mat3(transpose(inverse(model))) * vtxNormal;
	fragData.uv = vtxUV;
}
