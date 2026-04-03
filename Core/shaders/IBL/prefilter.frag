#include "Common.glsl"
#include "PBR.glsl"

// uniform /////////////////////////////////////////////////////////////////////
layout(location=8) uniform float u_roughness;
layout(location=9) uniform uint u_faceResolution;
layout(location=10) uniform uint u_sampleCount;

uniform samplerCube u_environmentMap;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 v_position;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

void main()
{
  vec3 N = normalize(v_position);
  vec3 R = N;
  vec3 V = R;

  //const uint SAMPLE_COUNT = 1024;
  vec3 prefilteredColor = vec3(0.0);
  float totalWeight = 0.0;
    
  for(uint i = 0; i < u_sampleCount; ++i)
  {
    // gets a sample vector that's biased towards the preferred alignment direction (importance sampling).
    vec2 Xi = Hammersley(i, u_sampleCount);
    vec3 H = ImportanceSampleGGX(Xi, N, u_roughness);
    vec3 L  = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL > 0.0)
    {
      // sample from the environment's mip level based on roughness
      float NdotH = max(dot(N, H), 0.0);
      float D   = GGXDistribution(NdotH, u_roughness);
      float HdotV = max(dot(H, V), 0.0);
      float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

      float saTexel  = 4.0 * PI / (6.0 * u_faceResolution * u_faceResolution);
      float saSample = 1.0 / (float(u_sampleCount) * pdf + 0.0001);

      float mipLevel = u_roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
      
      prefilteredColor += textureLod(u_environmentMap, L, mipLevel).rgb * NdotL;
      totalWeight      += NdotL;
    }
  }

  prefilteredColor = prefilteredColor / totalWeight;

  f_color = vec4(prefilteredColor, 1.0);
}