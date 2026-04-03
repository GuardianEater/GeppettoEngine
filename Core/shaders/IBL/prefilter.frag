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

float RadicalInverse_VdC(uint bits) 
{
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	// from spherical coordinates to cartesian
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space to world-space
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

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