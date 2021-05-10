#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_TexCoords;

out VS_OUT
{
	vec3 v_WorldPos;
	vec3 v_Normal;
	vec2 v_TexCoords;
	vec3 v_ViewPos;
	vec3 v_LightPositions[4];
} vs_out;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat3 u_NormalModel;

uniform vec3 u_ViewPos;
uniform vec3 u_LightPositions[4];

uniform bool u_TextureToggle;

uniform vec2 u_TilingFactor;

void main()
{
	vec4 worldPos = u_Model * vec4(a_Position, 1.0);

	vec3 N = normalize(u_NormalModel * a_Normal);
	vs_out.v_Normal = N;
	vs_out.v_TexCoords = a_TexCoords;
	if (u_TextureToggle)
	{
		if (u_TilingFactor.x > 0.0001 && u_TilingFactor.y > 0.0001)
			vs_out.v_TexCoords *= u_TilingFactor;

		vec3 T = u_NormalModel * a_Tangent;
		T = normalize(T - dot(N, T) * N);
	
		vec3 B = cross(N, T);

		mat3 TBN = transpose(mat3(T, B, N));
		vs_out.v_WorldPos = TBN * vec3(worldPos);
		vs_out.v_ViewPos = TBN * u_ViewPos;
		for (int i = 0; i < 4; i++)
			vs_out.v_LightPositions[i] = TBN * u_LightPositions[i];
	}
	else
	{
		vs_out.v_WorldPos = vec3(worldPos);
		vs_out.v_ViewPos = u_ViewPos;
		for (int i = 0; i < 4; i++)
			vs_out.v_LightPositions[i] = u_LightPositions[i];
	}

	gl_Position = u_ViewProjection * worldPos;
}