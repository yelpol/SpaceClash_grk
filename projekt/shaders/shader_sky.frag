#version 430 core

uniform sampler2D textureSampler;

in vec3 interpNormal;
in vec2 interpTexCoord;
in vec4 vertPosition_ws;

void main()
{
	vec3 color = texture2D(textureSampler, interpTexCoord).rgb;
	gl_FragColor = vec4(color, 1.0);
}
