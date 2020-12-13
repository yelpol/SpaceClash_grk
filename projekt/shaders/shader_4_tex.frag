#version 430 core

//uniform vec3 objectColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;
in vec2 interpVertexTex;

uniform sampler2D colorTexture;

void main()
{
	vec3 lightDir = normalize(lightPos-fragPos);
	vec3 V = normalize(cameraPos-fragPos);
	vec3 normal = normalize(interpNormal);
	vec3 R = reflect(-normalize(lightDir),normal);

	float specular = pow(max(0,dot(R,V)),10);
	float diffuse = max(0,dot(normal,normalize(lightDir)));

	vec2 modifiedVertexTex = vec2(interpVertexTex.x, 1.0 - interpVertexTex.y); //dla tekstury Ziemi
	vec4 textureColor = texture2D(colorTexture, modifiedVertexTex);
	gl_FragColor = vec4(textureColor.xyz * diffuse, 1.0);

	//gl_FragColor = vec4(mix(objectColor,objectColor*diffuse+vec3(1)*specular,0.9), 1.0);
}
