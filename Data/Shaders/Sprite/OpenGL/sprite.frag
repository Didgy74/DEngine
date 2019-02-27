#version 330

in vec2 fragUV;

uniform uint imageIndex;
uniform uvec2 subDivisions;

uniform sampler2D sampler;

void main()
{
	uint xIndex = imageIndex % subDivisions.x;
	uint yIndex = imageIndex / subDivisions.x;
	vec2 offset = vec2( xIndex * (1.0f / subDivisions.x), yIndex * (1.0f / subDivisions.y) );
	
	vec2 newUV = fragUV;
	newUV.x *= 1.0f / subDivisions.x;
	newUV.y *= 1.0f / subDivisions.y;
	
	newUV += offset;
	
	
	
	gl_FragColor = texture(sampler, newUV);
}
