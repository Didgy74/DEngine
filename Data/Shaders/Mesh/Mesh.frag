#version 420 core

layout(std140, binding = 1) uniform LightData
{
	uint pointLightCount;
	vec4 pointLightPos[10];
} lightData;

in FragData
{
	vec3 wsPosition;
	vec3 wsNormal;
	vec2 uv;
} fragData;

void main()
{
	float test = 0.0;
	for (int i = 0; i < lightData.pointLightCount; i++)
	{
		vec3 pointToLight = normalize(lightData.pointLightPos[i].xyz - fragData.wsPosition);
		test += max(dot(pointToLight, fragData.wsNormal), 0);
	}
	gl_FragColor = vec4(test, test, test, test);
}
