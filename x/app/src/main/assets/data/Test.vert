#version 450 core

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 matrix;
} cameraData;

layout(set = 1, binding = 0) uniform ObjectData
{
	mat4 matrix;
} objectData;

layout(location = 0) out vec2 uv;

vec3 positions[4] = { 
	vec3(-0.5, 0.5, 0.0), 
	vec3(-0.5, -0.5, 0.0), 
	vec3(0.5, 0.5, 0.0),
	vec3(0.5, -0.5, 0.0) };

vec2 uvs[4] = {
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0) };

void main()
{
	uv = uvs[gl_VertexIndex];

	gl_Position = cameraData.matrix * objectData.matrix * vec4(positions[gl_VertexIndex], 1.0);
	gl_Position.y = -gl_Position.y;
}
