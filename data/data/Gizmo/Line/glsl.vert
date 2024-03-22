#version 450 core

layout(location = 0) in vec3 vtxPosition;

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 matrix;
} cameraData;

layout(push_constant) uniform PushConstantData
{
	layout(offset = 0) mat4 objectMatrix;
} pushConstantData;

void main()
{
	gl_Position = cameraData.matrix * pushConstantData.objectMatrix * vec4(vtxPosition, 1.0);
	gl_Position.y = -gl_Position.y;
}