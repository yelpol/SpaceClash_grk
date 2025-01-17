#version 430 core

uniform sampler2D textureSampler;
uniform sampler2D normalSampler;
uniform vec3 lightDir;
uniform vec3 cameraPos;

in vec2 interpTexCoord;
in vec3 lightDirTS;
in vec3 viewDirTS;

void main()
{
	//vec2 modifiedTexCoord = vec2(interpTexCoord.x, 1.0 - interpTexCoord.y);

	vec3 L = normalize(-lightDirTS);
	vec3 V = normalize(viewDirTS);
	vec3 N = normalize((texture2D(normalSampler, interpTexCoord).rgb)*2-1);
	vec3 R = reflect(-normalize(L), N);

	float diffuse = max(0, dot(N, L));

	float specular_pow = 25;
	float specular = pow(max(0, dot(V, R)), specular_pow);

	vec3 color = texture2D(textureSampler, interpTexCoord).rgb;

	vec3 lightColor = vec3(1);
	vec3 shadedColor = color * diffuse + lightColor * specular;

	float ambient = 0.2;

	gl_FragColor = vec4(mix(color, shadedColor, 1.0 - ambient), 1.0);

}
