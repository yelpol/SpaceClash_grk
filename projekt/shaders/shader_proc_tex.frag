#version 430 core

uniform vec3 objectColor;
uniform vec3 extraColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;
in vec3 interpVertexPos;

void main()
{
	vec3 lightDir = normalize(lightPos-fragPos);
	vec3 V = normalize(cameraPos-fragPos);
	vec3 normal = normalize(interpNormal);
	vec3 R = reflect(-normalize(lightDir),normal);

	float specular = pow(max(0,dot(R,V)),5);
	float diffuse = max(0,dot(normal,normalize(lightDir)));

	//vec3 color = extraColor;
	//float t = sin(3.14159 * interpVertexPos.x / 0.2);
	//if (t < 0)
	//  color = objectColor;

	float t = (1 + sin(3.14159 * interpVertexPos.x / 0.2))/2;
	vec3 color = (1 - t) * objectColor + t * extraColor;

	gl_FragColor = vec4(mix(color, color*diffuse + vec3(1)*specular, 0.9), 1.0);
}
