#include "Common.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_depthTexture;
layout(binding=1) uniform sampler2D u_normalTexture;
layout(binding=2) uniform sampler2D u_colorTexture;
layout(binding=3) uniform sampler2D u_armTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color; // the resulting pixel color

vec3 GetWorldPosition(vec2 uv, float depth)
{
  vec2 ndcXY = uv * 2.0 - 1.0;
  float ndcZ = depth * 2.0 - 1.0;
  vec4 clip = vec4(ndcXY, ndcZ, 1.0);
  vec4 world = u_cams[u_camIndex].ipvMatrix * clip;
  world /= world.w;

  return world.xyz;
}

vec3 GetViewPosition(vec2 uv, float depth)
{
  vec3 worldPos = GetWorldPosition(uv, depth);
  vec4 viewPos = u_cams[u_camIndex].view * vec4(worldPos, 1.0);
  return viewPos.xyz;
}

const float c_radius = 1.0;
const uint c_samples = 32; // balanced samples with good jitter for smooth results
const float c_scale = 1.0;
const float c_contrast = 22.2;

void main() 
{
  float d = texture(u_depthTexture, v_uv).r;
  if (d >= 1.0) 
  {
    f_color = vec4(1.0);
    return;
  }

  vec3 P = GetWorldPosition(v_uv, d);
  vec3 N = normalize(texture(u_normalTexture, v_uv).rgb);

  float R = c_radius;
  uint n = c_samples;
  float c = 0.1 * R;
  float c2 = c * c;
  float delta = 0.001;

  float S = 0.0; // raw occlusion accumulator

  uint xp = uint(gl_FragCoord.x);
  uint yp = uint(gl_FragCoord.y);
  uint phi = ((30u * xp) ^ yp) + 10u * xp * yp;

  for (uint i = 0u; i < n; i++)
  {
    float alpha = (float(i) + 0.5) / float(n);
    float h = (alpha * R) / d;
    float theta = 2.0 * PI * alpha * ((7.0 * float(n)) / 9.0) + float(phi);

    vec2 uvi = v_uv + h * vec2(cos(theta), sin(theta));
    float di = texture(u_depthTexture, uvi).r;
    vec3 Pi = GetWorldPosition(uvi, di);

    vec3 wi = Pi - P; 

    float numerator = max(0.0, dot(N, wi) - (delta * di)) * step(0.0, R - length(wi));
    float denominator = max(c2, dot(wi, wi));

    S += numerator / denominator;
  }

  S *= (2.0 * PI * c) / float(n);
  float ao = clamp(1.0 - (c_scale * S), 0.0, 1.0);
  ao = pow(ao, c_contrast);

  f_color = vec4(vec3(ao), 1.0);
}