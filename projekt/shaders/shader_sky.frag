#version 430 core

uniform samplerCube cubemap;

//in vec2 interpTexCoord;
in vec3 interpTexCoord;

void main()
{
	//vec3 color = texture2D(textureSampler, interpTexCoord).rgb;
	//gl_FragColor = vec4(color, 1.0);
	gl_FragColor = texture(cubemap, interpTexCoord);
}
