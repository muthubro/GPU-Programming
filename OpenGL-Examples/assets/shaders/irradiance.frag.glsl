#version 450 core

in vec3 v_WorldPos;

out vec4 o_Color;

uniform samplerCube u_Environment;

const float PI = 3.14159265359;

void main()
{
	vec3 N = normalize(v_WorldPos);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, N);
	up = cross(N, right);

	float delta = 0.025;
	int samples = 0;
	vec3 irradiance = vec3(0.0);
	// Uniform sampling across the normal hemisphere and then averaged
	for (float phi = 0; phi < 2 * PI; phi += delta)
	{
		for (float theta = 0; theta < PI / 2; theta += delta)
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += texture(u_Environment, sampleVec).rgb;
			samples++;
		}
	}
	// PI appears when converting integral to summation
	//		1 division of phi   = 2 * PI / samples(phi)
	//		1 division of theta = PI / 2 * samples(phi)
	//
	// Irradiance = (c / PI) * Integral = (c * PI / samples) * Sum
	irradiance = PI * irradiance / float(samples);

	o_Color = vec4(irradiance, 1.0);
}