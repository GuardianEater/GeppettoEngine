#include "Common.glsl"
#include "PBR.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout (location = 0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location = 0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
layout(location=8) uniform uint u_sampleCount;

vec2 IntegrateBRDF(float NdotV, float roughness)
{
  vec3 V;
  V.x = sqrt(1.0 - NdotV * NdotV);
  V.y = 0.0;
  V.z = NdotV;

  float A = 0.0;
  float B = 0.0; 

  vec3 N = vec3(0.0, 0.0, 1.0);
  
  for(uint i = 0u; i < u_sampleCount; ++i)
  {
    // generates a sample vector that's biased towards the
    // preferred alignment direction (importance sampling).
    vec2 Xi = Hammersley(i, u_sampleCount);
    vec3 H = ImportanceSampleGGX(Xi, N, roughness);
    vec3 L = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(L.z, 0.0);
    float NdotH = max(H.z, 0.0);
    float VdotH = max(dot(V, H), 0.0);

    if(NdotL > 0.0)
    {
      float G = GeometrySmith(NdotV, NdotL, roughness);
      float G_Vis = (G * VdotH) / (NdotH * NdotV);
      float Fc = pow(1.0 - VdotH, 5.0);

      A += (1.0 - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }
  A /= float(u_sampleCount);
  B /= float(u_sampleCount);

  return vec2(A, B);
}

void main()
{
  // image 
  vec2 integratedBRDF = IntegrateBRDF(v_uv.x, v_uv.y);
  f_color = vec4(integratedBRDF, 0.0, 1.0);
}