#include "Common.glsl"

// uniform /////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_depthTexture;
layout(binding=1) uniform sampler2D u_normalTexture;
layout(binding=2) uniform sampler2D u_colorTexture;
layout(binding=3) uniform sampler2D u_armTexture;
layout(binding=4) uniform sampler2D u_ssaoTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// constants ///////////////////////////////////////////////////////////////////
const int c_kernelRadius = 3;
const float c_sigmaSpatial = 2.0;
const float c_sigmaRange = 0.01;

float Gaussian(float x, float sigma)
{
  return exp(-(x * x) / (2.0 * sigma * sigma));
}

float SpatialKernel(ivec2 offset)
{
  return Gaussian(length(vec2(offset)), c_sigmaSpatial);
}

float RangeKernel(float d, float di, vec3 N, vec3 Ni)
{
  float normalWeight = max(dot(Ni, N), 0.0);
  float depthDelta = di - d;
  float depthWeight = exp(-(depthDelta * depthDelta) / (2.0 * c_sigmaRange));
  float normalization = 1.0 / sqrt(2.0 * PI * c_sigmaRange);

  return normalWeight * normalization * depthWeight;
}

void main() 
{
  float d = texture(u_depthTexture, v_uv).r;
  if (d >= 1.0)
  {
    f_color = vec4(1.0);
    return;
  }

  vec3 N = normalize(texture(u_normalTexture, v_uv).xyz);
  vec2 texelSize = 1.0 / vec2(textureSize(u_ssaoTexture, 0));

  float weightedSum = 0.0;
  float totalWeight = 0.0;

  for (int y = -c_kernelRadius; y <= c_kernelRadius; ++y)
  {
    for (int x = -c_kernelRadius; x <= c_kernelRadius; ++x)
    {
      vec2 sampleUV = v_uv + vec2(float(x), float(y)) * texelSize;
      sampleUV = clamp(sampleUV, vec2(0.0), vec2(1.0));

      float di = texture(u_depthTexture, sampleUV).r;
      vec3 Ni = normalize(texture(u_normalTexture, sampleUV).xyz);
      float ao = texture(u_ssaoTexture, sampleUV).r;

      float spatialWeight = SpatialKernel(ivec2(x, y));
      float rangeWeight = RangeKernel(d, di, N, Ni);
      float w = spatialWeight * rangeWeight;

      weightedSum += ao * w;
      totalWeight += w;
    }
  }

  float blurredAO = texture(u_ssaoTexture, v_uv).r;
  if (totalWeight > 1e-6)
  {
    blurredAO = weightedSum / totalWeight;
  }

  f_color = vec4(vec3(clamp(blurredAO, 0.0, 1.0)), 1.0);
}  