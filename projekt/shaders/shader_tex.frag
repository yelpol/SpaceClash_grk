#version 430 core

uniform sampler2D textureSampler;
uniform vec3 lightDir;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec2 interpTexCoord;
in vec3 fragPos;

void main()
{
	vec2 modifiedTexCoord = vec2(interpTexCoord.x, 1.0 - interpTexCoord.y); // Poprawka dla tekstur Ziemi, ktore bez tego wyswietlaja sie 'do gory nogami'
	vec3 color = texture2D(textureSampler, modifiedTexCoord).rgb;
	vec3 normal = normalize(interpNormal);
	//vec3 lightDir = normalize(cameraPos - lightPos);

	float diffuse = max(dot(normal, -lightDir), 0.0);

	vec3 viewDir = normalize(cameraPos - fragPos);

	vec3 reflectDir = reflect(lightDir, normal);
	float specular = pow(max(dot(viewDir, reflectDir), 0.0), 25);

	gl_FragColor = vec4(color * diffuse + vec3(1.0) * specular, 1.0);

}
