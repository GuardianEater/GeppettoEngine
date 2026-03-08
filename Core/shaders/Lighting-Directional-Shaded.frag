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

// simple shadow calculation that returns a shadow value of either 1 or 0
float CalculateShadowDirectional(mat4 lightSpaceMatrix, vec3 fragPos, sampler2D shadowSampler)
{
  // compute the projected location of a fragment relateive to a light
  vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords = projCoords * 0.5 + 0.5;

  float shadow = 0.0;

  // check the bounds
  bool withinX = projCoords.x >= 0.0 && projCoords.x <= 1.0;
  bool withinY = projCoords.y >= 0.0 && projCoords.y <= 1.0;
  bool withinZ = projCoords.z <= 1.0;
  if (!withinX || !withinY || !withinZ)
    return shadow;
  
  float closestDepth = texture(shadowSampler, projCoords.xy).r;
  float currentDepth = projCoords.z;
  float bias = 0.0015;

  return (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
}

vec3 Cholesky(float m11, float m12, float m13, float m22, float m23, float m33, float z1, float z2, float z3)
{
  // from 2-MomentShadowMap
  // the variable names are following the paper
  float epsilon = 10e-5;
  // Because of round off errors, the three square roots below may be of negative values.
  // In that case set the result, a, d, or f, to a very small (but non-zero) value, say 10−4 

  float a = max(sqrt(m11), epsilon);
  float b = m12 / a;
  float c = m13 / a;
  float d = max(sqrt(m22 - b * b), epsilon);
  float e = (m23 - b * c) / d;
  float f = max(sqrt(m33 - c * c - e * e), epsilon);

  float c1 = z1 / a;
  float c2 = (z2 - b * c1) / d;
  float c3 = (z3 - c * c1 - e * c2) / f;

  c3 = c3 / f;
  c2 = (c2 - e * c3) / d;
  c1 = (c1 - b * c2 - c * c3) / a;

  return vec3(c1, c2, c3);
}

float CalculateShadowDirectional_Hamburger4MSM(mat4 lightSpaceMatrix, vec3 fragPos, sampler2D shadowSampler)
{
  // In the algorithm, b is the (z , z^2 , z^3 , z^4 ) vector retrieved from the blurred shadow map.
  vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords = projCoords * 0.5 + 0.5;

  if (projCoords.z > 1.0)
    return 1.0; 

  bool withinX = projCoords.x >= 0.0 && projCoords.x <= 1.0;
  bool withinY = projCoords.y >= 0.0 && projCoords.y <= 1.0;
  bool withinZ = projCoords.z >= 0.0 && projCoords.z <= 1.0;
  if (!withinX || !withinY || !withinZ)
    return 0.0;

  const vec4 b = texture(shadowSampler, projCoords.xy);
  const float a = 3 * 10e-5;
  const vec4 bp = mix(b, vec4(0.5), a);
  const float epsilon = 10e-5;

  float m11 = 1;
  float m12 = bp[0];
  //float m21 = bp[0];
  float m22 = bp[1];
  //float m31 = bp[1];
  float m13 = bp[1];
  float m23 = bp[2];
  //float m32 = bp[2];
  float m33 = bp[3];

  vec3 c = Cholesky(m11, m12, m13, m22, m23, m33, 1.0, projCoords.z, projCoords.z * projCoords.z);

  // solve for z2 and z3, using the quadratic equation
  float A = c.z;
  float B = c.y;
  float C = c.x;

  float z2 = 0, z3 = 0;

  if (abs(A) < epsilon)
  {
    // degenerates to linear: B*z + C = 0
    float zLin = (abs(B) < epsilon) ? 0.0 : (-C / B);
    z2 = zLin;
    z3 = zLin;
  }
  else
  {
    float discriminant = max(B * B - 4.0 * A * C, 0.0);
    float s = sqrt(discriminant);
    float inv2A = 0.5 / A;

    z2 = (-B - s) * inv2A;
    z3 = (-B + s) * inv2A;
  }

  // keep roots ordered
  if (z2 > z3)
  {
    float t = z2;
    z2 = z3;
    z3 = t;
  }

  if (projCoords.z <= z2)
  {
    return 0;
  }

  if (projCoords.z <= z3)
  {
    float num = projCoords.z * z3 - bp[0] * (projCoords.z + z3) + bp[1];
    float den = (z3 - z2) * (projCoords.z - z2);
    
    return num / den;
  }

  float num = z2 * z3 - bp[0] * (z2 + z3) + bp[1];
  float den = (projCoords.z - z2) * (projCoords.z - z3);

  return 1.0 - (num / den);
}

void main(void)
{
  // f_color = vec4(1.0, 0.0, 0.0, 1.0);
  // return; // do not do anything if there is nothing

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

  float shadow = CalculateShadowDirectional_Hamburger4MSM(lShadow.pvMatrix, position, sampler2D(lShadow.shadowMapHandle));
  //float shadow = CalculateShadowDirectional(lShadow.pvMatrix, position, sampler2D(lShadow.shadowMapHandle));
  // compute pbr
  vec3 finalColor = CalculatePBRDirectional(l, mat, normal, position, u_cams[u_camIndex].position.xyz);
  finalColor *= (1.0 - shadow);

  // Output with alpha for blending
  f_color = vec4(finalColor, 1.0);
  //return;
  //f_color = vec4(vec3(shadow), mat.color.a);
}