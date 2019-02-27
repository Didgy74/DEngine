#version 450

layout(set = 0, binding = 0) uniform Camera
{
	mat4 transform;
} camera;

layout(set = 0, binding = 1) uniform Model
{
	mat4 transform;
} model;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.transform * model.transform * vec4(inPosition, 1.0);
	gl_Position.y = -gl_Position.y;
	
	outUV = inUV;
}