#version 420 core

layout(std140, binding = 0) uniform CameraData
{
	vec4 wsPosition;
	mat4 viewProjection;
} cameraData;

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

layout(location = 0) out vec4 frag_color;

void main()
{
	vec3 pointToCamera = cameraData.wsPosition.xyz - fragData.wsPosition;
	vec3 pointToCameraDir = normalize(pointToCamera);

	float test = 0.0;
	for (int i = 0; i < lightData.pointLightCount; i++)
	{
		vec3 pointToLight = lightData.pointLightPos[i].xyz - fragData.wsPosition;
		vec3 pointToLightDir = normalize(pointToLight);
		
		float diff = max(0, dot(pointToLightDir, fragData.wsNormal));
		test += diff;
		
		vec3 lightToPointDir = -pointToLightDir;
		
		vec3 reflectDir = reflect(lightToPointDir, fragData.wsNormal);
		
		float spec = pow(max(dot(pointToCameraDir, reflectDir), 0.0), 50);
		
		test += spec;
		
	}
	frag_color = vec4(test);
}
