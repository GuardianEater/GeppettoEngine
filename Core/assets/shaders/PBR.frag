#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 worldPosition;    // the point that the light hits
layout(location=1) in vec3 worldNormal;      // the normal vector of the surface hit
layout(location=2) in vec2 uvOut;            // the uv coordinates of the surface hit
layout(location=3) flat in uint vObjectIndex; // the index of the currently being drawn object
layout(location=4) flat in uint vMeshIndex; // the index of the currently being drawn object
layout(location=5) flat in uint vMaterialIndex;   // the current material index into material uniforms

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 frag_color; // the resulting pixel color

// forward /////////////////////////////////////////////////////////////////////
vec3 CalculatePBRLightingTotal();

// all of the samples in a single struct
struct CurrentSample
{
  float ao;       // ambientOcclusion
  float roughness;
  float metallic; 
  vec4 color;
};

CurrentSample currentSample;

void main(void)
{
  const ObjectUniforms object = objectUniforms[vObjectIndex];
  const PBRMaterial material  = materialUniforms[vMaterialIndex];

  // if the texture handle is null, then use material.color else sample from the texture
  currentSample.ao        = material.aoTextureHandle        == uvec2(0,0) ? material.ao        : texture(sampler2D(material.aoTextureHandle), uvOut).r;
  currentSample.roughness = material.roughnessTextureHandle == uvec2(0,0) ? material.roughness : texture(sampler2D(material.roughnessTextureHandle), uvOut).r;
  currentSample.metallic  = material.metallicTextureHandle  == uvec2(0,0) ? material.metallic  : texture(sampler2D(material.metallicTextureHandle), uvOut).r;
  currentSample.color     = material.colorTextureHandle     == uvec2(0,0) ? material.color     : texture(sampler2D(material.colorTextureHandle), uvOut);

  float alpha = currentSample.color.a;
  const float ALPHA_CUTOFF = 0.02;
  if (alpha < ALPHA_CUTOFF)
    discard;

  vec3 finalColor = CalculatePBRLightingTotal();

  // Output with alpha for blending
  frag_color = vec4(finalColor, alpha);
}

// calculates F in the pbr equation
vec3 SchlickFresnel(float vDotH)
{
  const float dielectricDefault = 0.04;
  vec3 F0 = vec3(dielectricDefault);
  F0 = mix(F0, vec3(currentSample.color), currentSample.metallic);

  const float clamped = clamp(1.0 - vDotH, 0.0, 1.0);
  const vec3 result = F0 + (1.0 - F0) * pow(clamped, 5);

  return result;
}

float GeometrySchlickGGX(float dp)
{
  float k = (currentSample.roughness + 1.0) * (currentSample.roughness + 1.0) / 8.0;
  float denom = dp * (1.0 - k) + k;
  
  return dp / denom;
}

// calculates g in the pbr equation
float GeometrySmith(float nDotV, float nDotL)
{
  return GeometrySchlickGGX(nDotV) * GeometrySchlickGGX(nDotL);
}

// calculates D in the pbr equation
float GGXDistribution(float nDotH)
{
  float alpha2 = pow(currentSample.roughness, 4);
  float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
  float ggx = alpha2 / (PI * d * d);

  return ggx;
}

// only for point lights
vec3 CalculatePBRPoint(PointLightUniforms light, vec3 n, vec3 objectColor)
{
  vec3 l = vec3(0.0);

  l = light.position - worldPosition;
  float lightToPixelDist = length(l); // before normalizing get the length
  l = normalize(l);
  
  float attenuation = 1.0 / (lightToPixelDist * lightToPixelDist);
  vec3 radiance = light.color * attenuation * light.intensity * 100.0;

  vec3 v = normalize(cameraUniforms[cameraIndex].camPosition.xyz - worldPosition); // view vector
  vec3 h = normalize(v + l); // half vector

  float nDotH = max(dot(n, h), 0.0); // clamp to 0 for values less than 0
  float vDotH = max(dot(v, h), 0.0);
  float nDotL = max(dot(n, l), 0.0);
  float nDotV = max(dot(n, v), 0.0);

  float D = GGXDistribution(nDotH); // normal distribution
  float G = GeometrySmith(nDotV, nDotL); // geometry
  vec3 F = SchlickFresnel(vDotH); // fresnel

  vec3 ks = F; // specular coefficient
  vec3 kd = vec3(1.0) - ks; // diffuse coefficient
  kd *= 1.0 - currentSample.metallic;

  vec3 numerator = D * G * F;
  float denominator = 4.0 * nDotL * nDotV + 0.0001; // prevents division by 0

  vec3 specularBRDF = numerator / denominator;
  vec3 diffuseBRDF = kd * objectColor / PI;

  vec3 finalColor = (diffuseBRDF + specularBRDF) * radiance * nDotL;

  return finalColor;
}

// Simple global directional light (hardcoded for quick testing)
vec3 CalculatePBRDirectional(DirectionalLightUniforms light, vec3 n, vec3 objectColor)
{
  // Direction points from light toward the scene; light comes FROM -dir
  const vec3 dir = normalize(light.direction);
  const vec3 lightColor = light.color;
  const float intensity = light.intensity;

  vec3 l = normalize(-dir);
  vec3 v = normalize(cameraUniforms[cameraIndex].camPosition.xyz - worldPosition);
  vec3 h = normalize(v + l);

  float nDotH = max(dot(n, h), 0.0);
  float vDotH = max(dot(v, h), 0.0);
  float nDotL = max(dot(n, l), 0.0);
  float nDotV = max(dot(n, v), 0.0);

  float D = GGXDistribution(nDotH);
  float G = GeometrySmith(nDotV, nDotL);
  vec3 F = SchlickFresnel(vDotH);

  vec3 ks = F;
  vec3 kd = (vec3(1.0) - ks) * (1.0 - currentSample.metallic);

  vec3 numerator = D * G * F;
  float denominator = 4.0 * nDotL * nDotV + 0.0001;

  vec3 specularBRDF = numerator / denominator;
  vec3 diffuseBRDF = kd * objectColor / PI;

  vec3 radiance = lightColor * intensity; // no attenuation for directional

  return (diffuseBRDF + specularBRDF) * radiance * nDotL;
}

// Builds a world-space normal using a tangent-space normal map if present.
// Computes TBN per-fragment via derivatives to avoid requiring mesh tangents.
vec3 ComputeShadedNormal()
{
  vec3 n = normalize(worldNormal);

  const PBRMaterial material = materialUniforms[vMaterialIndex];
  if (material.normalTextureHandle != uvec2(0,0))
  {
    // Sample and decode normal from [0,1] -> [-1,1]
    vec3 mapN = texture(sampler2D(material.normalTextureHandle), uvOut).xyz * 2.0 - 1.0;

    // Derivative-based TBN construction in world space
    vec3 dp1 = dFdx(worldPosition);
    vec3 dp2 = dFdy(worldPosition);
    vec2 duv1 = dFdx(uvOut);
    vec2 duv2 = dFdy(uvOut);

    vec3 t = normalize(duv2.y * dp1 - duv1.y * dp2);
    // Orthonormalize T against N, then derive B
    t = normalize(t - n * dot(n, t));
    vec3 b = normalize(cross(n, t));

    mat3 tbn = mat3(t, b, n);
    n = normalize(tbn * mapN);
  }

  return n;
}

vec3 CalculatePBRLightingTotal()
{    
  vec3 n = ComputeShadedNormal();
  vec3 color = vec3(0.0); // the final color of the fragment

  // point lights
  for (int i = 0; i < pointLightCount; i++) 
  {
    color += CalculatePBRPoint(pointLightUniforms[i], n, currentSample.color.rgb);
  }

  // directional lights
  for (int i = 0; i < directionalLightCount; i++) 
  {
    color += CalculatePBRDirectional(directionalLightUniforms[i], n, currentSample.color.rgb);
  }

  // ambient lighting
  vec3 ambient = vec3(0.8) * currentSample.color.rgb * currentSample.ao;
  color += ambient;

  // HDR tone mapping
  //color /= (color + vec3(1.0));

  //gamma correction
  //color = pow(color, vec3(1.0 / 2));

  return color;
}