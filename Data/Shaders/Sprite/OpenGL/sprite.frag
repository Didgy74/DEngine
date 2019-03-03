#version 330

in vec2 fragUV;

uniform sampler2D sampler;

void main()
{	
	gl_FragColor = texture(sampler, fragUV);
}
