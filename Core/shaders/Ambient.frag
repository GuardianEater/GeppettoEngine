#include "Common.glsl"
#include "PBR.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
uniform sampler2D u_depthTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_colorTexture;
uniform sampler2D u_armTexture;
uniform samplerCube u_irradianceMap;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color; // the resulting pixel color

vec3 GetPosition(vec2 uv, float depth)
{
	vec2 ndcXY = uv * 2.0 - 1.0;
	float ndcZ = depth * 2.0 - 1.0;
	vec4 clip = vec4(ndcXY, ndcZ, 1.0);
	vec4 world = u_cams[u_camIndex].ipvMatrix * clip;
	world /= world.w;

	return world.xyz;
}

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_depthTexture, 0));
	float depth = texture(u_depthTexture, uv).x;

	// No geometry was written to this pixel in the gbuffer.
	if (depth >= 1.0)
	{
		f_color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 arm = texture(u_armTexture, uv).xyz;
	vec3 normal = normalize(texture(u_normalTexture, uv).xyz);
	// vec3 position = GetPosition(uv, depth);
	// vec3 view = normalize(u_cams[u_camIndex].position.xyz - position);

	MaterialSample mat;
	mat.color     = texture(u_colorTexture, uv);
	mat.ao        = arm.x;
	mat.roughness = arm.y;
	mat.metallic  = arm.z;

  // const float dielectricDefault = 0.04;
  // vec3 F0 = vec3(dielectricDefault);
  // F0 = mix(F0, mat.color.rgb, mat.metallic);

  // vec3 kS = SchlickFresnelRoughness(max(dot(normal, view), 0.0), F0, mat.roughness); 
  // vec3 kD = 1.0 - kS;
	vec3 irradiance = texture(u_irradianceMap, normal).rgb;
	vec3 ambient = irradiance * mat.color.rgb * mat.ao;

	f_color = vec4(ambient, mat.color.a);
}