#version 450 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

uniform mat4 u_Model;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_Model * vec4(a_Position, 0.0f, 1.0f);
}