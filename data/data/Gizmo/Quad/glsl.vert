#version 450 core

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 matrix;
} cameraData;

layout(push_constant) uniform PushConstantData
{
	layout(offset = 0) mat4 objectMatrix;
} pushConstantData;

vec3 positions[4] = { 
	vec3(-0.5, 0.5, 0.0), 
	vec3(-0.5, -0.5, 0.0), 
	vec3(0.5, 0.5, 0.0),
	vec3(0.5, -0.5, 0.0) };

void main()
{
	gl_Position = cameraData.matrix * pushConstantData.objectMatrix * vec4(positions[gl_VertexIndex], 1.0);
	gl_Position.y = -gl_Position.y;
}