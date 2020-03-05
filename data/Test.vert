#version 450 core

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 matrix;
} cameraData;

vec3 positions[4] = { 
	vec3(-0.5, 0.5, 0.0), 
	vec3(-0.5, -0.5, 0.0), 
	vec3(0.5, 0.5, 0.0),
	vec3(0.5, -0.5, 0.0)
};

void main()
{
	gl_Position = cameraData.matrix * vec4(positions[gl_VertexIndex], 1.0);
}
