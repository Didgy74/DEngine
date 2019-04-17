#version 420 core

layout(std140, binding = 0) uniform CameraData
{
	vec3 wsPosition;
	mat4 viewProjection;
} cameraData;

layout(std140, binding = 1) uniform LightData
{
	vec4 ambientLight;
	uint pointLightCount;
	vec4 pointLightIntensity[10];
	vec4 pointLightPos[10];
} lightData;

in FragData
{
	vec3 wsPosition;
	vec3 wsNormal;
	vec2 uv;
} fragData;

layout(location = 0) out vec4 frag_color;

uniform sampler2D myTexture;

void main()
{
	vec3 pointToCamera = cameraData.wsPosition - fragData.wsPosition;
	vec3 pointToCameraDir = normalize(pointToCamera);

	vec3 diffuse;
	vec3 specular;
	for (int i = 0; i < lightData.pointLightCount; i++)
	{
		vec3 pointToLight = lightData.pointLightPos[i].xyz - fragData.wsPosition;
		vec3 pointToLightDir = normalize(pointToLight);
		
		diffuse += max(0, dot(pointToLightDir, fragData.wsNormal)) * vec3(lightData.pointLightIntensity[i]);
		
		vec3 lightToPointDir = -pointToLightDir;
		
		vec3 reflectDir = reflect(lightToPointDir, fragData.wsNormal);
		
		const float coefficient = 50;
		specular += pow(max(dot(pointToCameraDir, reflectDir), 0.0), coefficient) * vec3(1);
	}
	
	vec3 resultColor =  diffuse;
	resultColor = resultColor * texture(myTexture, fragData.uv).rgb;
	frag_color = vec4(resultColor, 1.0);
}
