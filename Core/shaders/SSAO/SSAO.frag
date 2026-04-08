#include "Common.glsl"
#include "PBR.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_depthTexture;
layout(binding=1) uniform sampler2D u_normalTexture;
layout(binding=2) uniform sampler2D u_colorTexture;
layout(binding=3) uniform sampler2D u_armTexture;
layout(binding=4) uniform sampler2D u_noiseTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec2 v_uv;

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

const int kernelSize = 64;
const float radius = 0.5;
const float bias = 0.025;

void main()
{
  const vec2 screenSize = vec2(textureSize(u_depthTexture, 0));
  const vec2 noiseSize = vec2(textureSize(u_noiseTexture, 0));
  const vec2 noiseScale = vec2(screenSize.x/noiseSize.x, screenSize.y/noiseSize.y); 

  // get input for SSAO algorithm
  float depth = texture(u_depthTexture, v_uv).x;  
  vec3 position = GetPosition(v_uv, depth);
  vec3 normal = normalize(texture(u_normalTexture, v_uv).rgb);
  vec3 randomVec = normalize(texture(u_noiseTexture, v_uv * noiseScale).xyz);

  // create TBN change-of-basis matrix: from tangent-space to view-space
  vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
  vec3 bitangent = cross(normal, tangent);
  mat3 TBN = mat3(tangent, bitangent, normal);

  // iterate over the sample kernel and calculate occlusion factor
  float occlusion = 0.0;
  for(int i = 0; i < kernelSize; ++i)
  {
    // get sample position
    vec3 samplePos = TBN * u_ssaoSamples[i]; // from tangent to view-space
    samplePos = position + samplePos * radius; 
    
    // project sample position (to sample texture) (to get position on screen/texture)
    vec4 offset = vec4(samplePos, 1.0);
    offset = u_cams[u_camIndex].perspective * offset; // from view to clip-space
    offset.xyz /= offset.w; // perspective divide
    offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
    
    // get sample depth
    float sampleDepth = texture(u_depthTexture, offset.xy).r; // get depth value of kernel sample
    
    // range check & accumulate
    float rangeCheck = smoothstep(0.0, 1.0, radius / abs(position.z - sampleDepth));
    occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
  }
  occlusion = 1.0 - (occlusion / kernelSize);
  
  f_color = vec4(vec3(occlusion), 1.0);
}
