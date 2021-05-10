#version 450 core

in vec3 v_TexCoord;

layout(location = 0) out vec4 o_Color;

uniform samplerCube u_CubeMap;
uniform float u_Mip;

void main()
{
	vec3 color = textureLod(u_CubeMap, v_TexCoord, u_Mip).rgb;
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	o_Color = vec4(color, 1.0);
}