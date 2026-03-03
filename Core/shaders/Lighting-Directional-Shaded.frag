#include "Common.glsl"
#include "PBR.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
uniform sampler2D u_depthTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_colorTexture;
uniform sampler2D u_armTexture;

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) flat in uint v_InstanceID;

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
  f_color = vec4(1.0, 0.0, 0.0, 1.0);
  return; // do not do anything if there is nothing

  // reconstructs uv from frag position and texture size
  // (any texture from the gbuffer would work they are all the same size)
  vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_depthTexture, 0));
  float depth = texture(u_depthTexture, uv).x;  
  if (depth >= 1.0) 
  {
    f_color = vec4(0.0, 0.0, 0.0, 1.0);
    return; // do not do anything if there is nothing
  }

  // reconstructs position from uv and depth
  vec3 position = GetPosition(uv, depth);
  DirectionalLightShadowUniforms lShadow = u_directionalLightShadows[v_InstanceID];
  DirectionalLightUniforms l = lShadow.light;

  // extracts materials from the gbuffer
  vec3 arm = texture(u_armTexture, uv).xyz;
  vec3 normal = texture(u_normalTexture, uv).xyz;

  MaterialSample mat;
  mat.color     = texture(u_colorTexture, uv);
  mat.ao        = arm.x;
  mat.roughness = arm.y;
  mat.metallic  = arm.z;

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