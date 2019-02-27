#version 450

layout(set = 0, binding = 0) uniform CameraModel
{
    mat4 model;
} camera;

layout(set = 0, binding = 1) uniform Transform
{
	mat4 model;
} transform;

layout(location = 0) in vec3 inPosition;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.model * transform.model * vec4(inPosition, 1.0);
	gl_Position.y = -gl_Position.y;
}