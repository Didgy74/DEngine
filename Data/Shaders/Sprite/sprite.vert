#version 330

in vec3 position;
in vec2 texCoord;
in vec3 normal;

out vec2 fragUV;

uniform mat4 model;
uniform mat4 viewProjection;

void main()
{
	gl_Position = viewProjection * model * vec4(position, 1.0);
	
	fragUV = texCoord;
}
