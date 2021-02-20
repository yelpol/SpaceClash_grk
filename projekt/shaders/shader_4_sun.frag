#version 430 core

uniform sampler2D textureSampler;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;
in vec2 interpTexCoord;

void main()
{	
	vec3 color = texture2D(textureSampler, interpTexCoord).rgb;
	vec3 normal = normalize(interpNormal);
	vec3 viewDir = normalize(cameraPos - fragPos);
	float diffuse = max(dot(normal, viewDir), 0.0);
	gl_FragColor = vec4(color * diffuse, 1.0);

}
