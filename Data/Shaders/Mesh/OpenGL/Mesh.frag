#version 330 core

in FragData
{
	vec3 wsPosition;
	vec3 wsNormal;
	vec2 uv;
} fragData;

vec3 lightPos = vec3(4.0, 5.0, 6.0);

void main()
{
	vec3 pointToLight = normalize(lightPos - fragData.wsPosition);
	float test = max(dot(pointToLight, fragData.wsNormal), 0);
	gl_FragColor = vec4(test, test, test, test);
}
