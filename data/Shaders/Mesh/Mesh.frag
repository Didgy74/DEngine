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

float constant = 1.0;
float linear = 0.09;
float quadratic = 0.032;

void main()
{
	vec3 color = texture(myTexture, fragData.uv).rgb;

	vec3 pointToCamera = cameraData.wsPosition - fragData.wsPosition;
	vec3 pointToCameraDir = normalize(pointToCamera);

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	for (int i = 0; i < lightData.pointLightCount; i++)
	{
		vec3 pointToLight = lightData.pointLightPos[i].xyz - fragData.wsPosition;
		float distance = length(pointToLight);
		float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
	
		ambient += attenuation * lightData.pointLightIntensity[i].xyz * 0.1f;
		
		
		vec3 pointToLightDir = normalize(pointToLight);
		
		diffuse += max(0, dot(pointToLightDir, fragData.wsNormal)) * attenuation * lightData.pointLightIntensity[i].xyz;
		
		
		vec3 lightToPointDir = normalize(-pointToLightDir);
		
		vec3 reflectDir = normalize(reflect(lightToPointDir, fragData.wsNormal));
		
		float coefficient = 100;
		specular += vec3(pow(max(dot(pointToCameraDir, reflectDir), 0.0), coefficient) * lightData.pointLightIntensity[i] * attenuation);
	}
	
	vec3 resultIntensity = (ambient + diffuse)  * color + specular;
	vec3 resultColor = resultIntensity;
	frag_color = vec4(resultColor, 1.0);
}
