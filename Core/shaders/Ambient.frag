#include "Common.glsl"
#include "PBR.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_depthTexture;
layout(binding=1) uniform sampler2D u_normalTexture;
layout(binding=2) uniform sampler2D u_colorTexture;
layout(binding=3) uniform sampler2D u_armeTexture;
layout(binding=4) uniform sampler2D u_brdflut;
layout(binding=5) uniform samplerCube u_prefilterMap;
layout(binding=6) uniform samplerCube u_irradianceMap;
layout(binding=7) uniform sampler2D u_ambientOcclusion;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

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
	float depth = texture(u_depthTexture, v_uv).x;

	// No geometry was written to this pixel in the gbuffer.
	if (depth >= 1.0)
	{
		f_color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec4 arme = texture(u_armeTexture, v_uv);
	vec3 normal = normalize(texture(u_normalTexture, v_uv).xyz);
	vec3 position = GetPosition(v_uv, depth);
	vec3 view = normalize(u_cams[u_camIndex].position.xyz - position);
  vec3 reflection = reflect(-view, normal);

	MaterialSample mat;
	mat.color     = texture(u_colorTexture, v_uv);
	mat.ao        = arme.x;
	mat.roughness = arme.y;
	mat.metallic  = arme.z;
  mat.emission  = arme.w;

  const float dielectricDefault = 0.04;
  vec3 F0 = vec3(dielectricDefault);
  F0 = mix(F0, mat.color.rgb, mat.metallic);

  vec3 F =  SchlickFresnelRoughness(max(dot(normal, view), 0.0), F0, mat.roughness); ;
  
  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - mat.metallic;
  
	vec3 irradiance = texture(u_irradianceMap, normal).rgb;
  vec3 diffuse = irradiance * mat.color.rgb;

  const float MAX_REFLECTION_LOD = 4.0;
  vec3 prefilteredColor = textureLod(u_prefilterMap, reflection,  mat.roughness * MAX_REFLECTION_LOD).rgb;     
  vec2 brdf  = texture(u_brdflut, vec2(max(dot(normal, view), 0.0), mat.roughness)).rg;
  vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

  //vec3 ambient = (kD * diffuse + specular) * texture(u_ambientOcclusion, v_uv).r;
  vec3 ambient = (kD * diffuse + specular) * mat.ao;

	f_color = vec4(ambient, mat.color.a);
}