#version 430 core

uniform vec3 objectColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;

void main()
{
	vec3 normal = normalize(interpNormal);
	vec3 lightDir = normalize(cameraPos - lightPos);
	float diffuse = max(dot(normal, -lightDir), 0.0);
	vec3 viewDir = normalize(cameraPos - fragPos);
	vec3 reflectDir = reflect(lightDir, normal);
	float specular = pow(max(dot(viewDir, reflectDir), 0.0), 8);

	gl_FragColor = vec4(objectColor * diffuse + vec3(1.0) * specular, 1.0);
}
