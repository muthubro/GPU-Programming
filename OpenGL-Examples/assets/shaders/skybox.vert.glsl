#version 450 core

layout(location = 0) in vec3 a_Position;

out vec3 v_TexCoord;

uniform mat4 u_ViewProjection;

void main()
{
	v_TexCoord = a_Position;
	vec4 position = u_ViewProjection * vec4(a_Position, 1.0f);
	gl_Position = position.xyww;
}