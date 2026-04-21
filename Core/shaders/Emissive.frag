#include "Common.glsl"
#include "PBR.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_depthTexture;
layout(binding=1) uniform sampler2D u_normalTexture;
layout(binding=2) uniform sampler2D u_colorTexture;
layout(binding=3) uniform sampler2D u_armeTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color; // the resulting pixel color

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
  vec4 color = texture(u_colorTexture, v_uv);
	MaterialSample mat;
	mat.color     = color;
	mat.ao        = arme.x;
	mat.roughness = arme.y;
	mat.metallic  = arme.z;
  mat.emission  = arme.w;

	vec3 emissiveColor = mat.color.rgb * mat.emission;

	f_color = vec4(emissiveColor, 1.0);
}