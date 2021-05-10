#version 450 core

in vec3 v_WorldPos;

out vec4 o_Color;

uniform sampler2D u_EquirectangularMap;

const vec2 coefs = vec2(0.15915, 0.31831);

vec2 SphericalToRectangular(vec2 angles)
{
	vec2 coords = angles * coefs;
	return coords + vec2(0.5);
}

void main()
{
	vec3 point = normalize(v_WorldPos);
	vec2 angles = vec2(atan(point.z, point.x), asin(point.y));
	vec2 texCoords = SphericalToRectangular(angles);

	vec3 color = texture(u_EquirectangularMap, texCoords).rgb;
	o_Color = vec4(color, 1.0);
}