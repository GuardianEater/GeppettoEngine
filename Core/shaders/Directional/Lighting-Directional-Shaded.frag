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

  // reconstructs uv from frag position and texture size
  // (any texture from the gbuffer would work they are all the same size)
  float depth = texture(u_depthTexture, v_uv).x;  
  if (depth >= 1.0) 
  {
    f_color = vec4(0.0, 0.0, 0.0, 1.0);
    return; // do not do anything if there is nothing
  }

  // reconstructs position from uv and depth
  vec3 position = GetPosition(v_uv, depth);
  DirectionalLightShadowUniforms lShadow = u_directionalLightShadows[v_InstanceID];
  DirectionalLightUniforms l = lShadow.light;

  // extracts materials from the gbuffer
  vec4 arme = texture(u_armeTexture, v_uv);
  vec3 normal = texture(u_normalTexture, v_uv).xyz;

  MaterialSample mat;
  mat.color     = texture(u_colorTexture, v_uv);
  mat.ao        = arme.x;
  mat.roughness = arme.y;
  mat.metallic  = arme.z;
  mat.emission  = arme.w;

  // calculate shadow
  vec4 fragPosLightSpace = lShadow.pvMatrix * vec4(position, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords = projCoords * 0.5 + 0.5;
  float shadow = 0.0;
  if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
      projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
      projCoords.z <= 1.0)
  {
    float closestDepth = texture(sampler2D(lShadow.shadowMapHandle), projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.0015;
    shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
  }

  // compute pbr
  vec3 finalColor = CalculatePBRDirectional(l, mat, normal, position, u_cams[u_camIndex].position.xyz);
  finalColor *= (1.0 - shadow);

  // Output with alpha for blending
  f_color = vec4(finalColor, mat.color.a);
}