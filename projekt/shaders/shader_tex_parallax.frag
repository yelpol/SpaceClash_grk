#version 430 core

uniform sampler2D textureSampler;
uniform sampler2D normalSampler;
uniform sampler2D depthSampler;

uniform vec3 lightDir;
uniform vec3 cameraPos;

in vec2 interpTexCoord;
in vec3 lightDirTS;
in vec3 viewDirTS;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0)); 

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.5;

    vec2 P = viewDir.xy / viewDir.z * 0.1;
    vec2 deltaTexCoords = P / numLayers;
	
	vec2  currentTexCoords = texCoords;
	float currentDepthMapValue = texture2D(depthSampler, currentTexCoords).r;
  
	while(currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = texture2D(depthSampler, currentTexCoords).r;
		currentLayerDepth += layerDepth;  
	}

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
	
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture2D(depthSampler, prevTexCoords).r - currentLayerDepth + layerDepth;
	
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;  
}

void main()
{
	vec3 V = normalize(viewDirTS);
	vec2 texCoords = interpTexCoord;

	texCoords = ParallaxMapping(interpTexCoord, V);
	if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
		discard;
	
	vec3 N = texture2D(normalSampler, texCoords).rgb;
	N = normalize(N * 2.0 - 1.0);

	vec3 color = texture2D(textureSampler, texCoords).rgb;
	
	vec3 ambient = 0.1 * color;
	
	vec3 L = normalize(-lightDirTS);
	float diff = max(0, dot(N, L));
	vec3 diffuse = diff * color;
	
	vec3 R = reflect(-normalize(L), N);
	
	vec3 halfwayDir = normalize(L + V);  
	float spec = pow(max(dot(N, halfwayDir), 0.0), 32.0);
	vec3 specular = vec3(0.2) * spec;
	
	gl_FragColor = vec4(ambient + diffuse + specular, 1.0);

}
