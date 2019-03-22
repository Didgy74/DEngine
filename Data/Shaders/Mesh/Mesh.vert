#version 420 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

layout(std140, binding = 0) uniform CameraData
{
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
	gl_Position = cameraData.viewProjection * model * vec4(position, 1.0);
	
	fragData.wsPosition = vec4(model * vec4(position, 1.0)).xyz;
	fragData.wsNormal = vec4(model * vec4(normal, 0.0)).xyz;
	fragData.uv = texCoord;
}
