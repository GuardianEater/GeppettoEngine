#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout (location = 0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location = 0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
uniform samplerCube u_cubeMap;

vec3 EquirectangularToDirection(vec2 uv)
{
	float theta = uv.x * 2.0 * PI - PI;
	float phi = uv.y * PI - (0.5 * PI);

	float cosPhi = cos(phi);
	return vec3(
		cosPhi * cos(theta),
		sin(phi),
		cosPhi * sin(theta)
	);
}

void main()
{
	vec3 direction = normalize(EquirectangularToDirection(v_uv));
	vec3 color = texture(u_cubeMap, direction).rgb;

	f_color = vec4(color, 1.0);
}
