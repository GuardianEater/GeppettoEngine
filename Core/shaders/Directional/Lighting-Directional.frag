#include "Common.glsl"
#include "PBR.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_depthTexture;
layout(binding=1) uniform sampler2D u_normalTexture;
layout(binding=2) uniform sampler2D u_colorTexture;
layout(binding=3) uniform sampler2D u_armeTexture;

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;
layout(location=1) flat in uint v_InstanceID;

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

void main(void)
{
  // f_color = vec4(1.0, 0.0, 0.0, 1.0);
  // return; // do not do anything if there is nothing

  float depth = texture(u_depthTexture, v_uv).x;  
  if (depth >= 1.0) 
  {
    f_color = vec4(0.0, 0.0, 0.0, 1.0);
    return; // do not do anything if there is nothing
  }

  // reconstructs position from uv and depth
  vec3 position = GetPosition(v_uv, depth);
  DirectionalLightUniforms l = u_directionalLights[v_InstanceID];

  // extracts materials from the gbuffer
  vec4 arme = texture(u_armeTexture, v_uv);
  vec3 normal = texture(u_normalTexture, v_uv).xyz;

  MaterialSample mat;
  mat.color     = texture(u_colorTexture, v_uv);
  mat.ao        = arme.x;
  mat.roughness = arme.y;
  mat.metallic  = arme.z;
  mat.emission  = arme.w;

  // compute pbr
  vec3 finalColor = CalculatePBRDirectional(l, mat, normal, position, u_cams[u_camIndex].position.xyz);

  // Output with alpha for blending
  f_color = vec4(finalColor, mat.color.a);
}