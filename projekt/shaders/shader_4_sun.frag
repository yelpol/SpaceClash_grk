#version 430 core

uniform sampler2D textureSampler;
uniform vec3 objectColor;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;
in vec2 interpTexCoord;

void main()
{
	vec2 modifiedTexCoord = vec2(interpTexCoord.x, 1.0 - interpTexCoord.y); // Poprawka dla tekstur Ziemi, ktore bez tego wyswietlaja sie 'do gory nogami'
	vec3 color = texture2D(textureSampler, modifiedTexCoord).rgb;

	vec3 normal = normalize(interpNormal);

	vec3 viewDir = normalize(cameraPos - fragPos);

	float diffuse = max(dot(normal, viewDir), 0.0);

	gl_FragColor = vec4(color * diffuse, 1.0);
}
