/*//////////////////////////////////////////////////////////////////////////////
 * author : 2018t
 * date   : May 2025
 *
 * brief  : a file with definitions for various PBR calculation functions
/*//////////////////////////////////////////////////////////////////////////////

// all of the samples in a single struct
struct MaterialSample
{
  float ao;       // ambientOcclusion
  float roughness;
  float metallic; 
  vec4 color;
};

// calculates F in the pbr equation
vec3 SchlickFresnel(float vDotH, vec3 color, float metallic)
{
  const float dielectricDefault = 0.04;
  vec3 F0 = vec3(dielectricDefault);
  F0 = mix(F0, color, metallic);

  const float clamped = clamp(1.0 - vDotH, 0.0, 1.0);
  const vec3 result = F0 + (1.0 - F0) * pow(clamped, 5);

  return result;
}

float GeometrySchlickGGX(float dp, float roughness)
{
  float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
  float denom = dp * (1.0 - k) + k;
  
  return dp / denom;
}

// calculates g in the pbr equation
float GeometrySmith(float nDotV, float nDotL, float roughness)
{
  return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

// calculates D in the pbr equation
float GGXDistribution(float nDotH, float roughness)
{
  float alpha2 = pow(roughness, 4);
  float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
  float ggx = alpha2 / (PI * d * d);

  return ggx;
}

/// Computes the pixel of a fragment given input information
/// light - the light currently being used
/// mat   - the material sampled at the current fragment
/// fragN - the normal at the fragment location
/// fragP - the fragment position in world space
/// camP  - position of the camera
vec3 CalculatePBRPoint(PointLightUniforms light, MaterialSample mat, vec3 fragN, vec3 fragP, vec3 camP)
{
  vec3 l = light.position - fragP;
  float lightToPixelDist = length(l); // before normalizing get the length
  l = normalize(l);
  
  float attenuation = light.intensity / (lightToPixelDist * lightToPixelDist);
  vec3 radiance = light.color * attenuation;

  vec3 v = normalize(camP - fragP); // view vector
  vec3 h = normalize(v + l); // half vector
  vec3 n = normalize(fragN);

  float nDotH = max(dot(n, h), 0.0); // clamp to 0 for values less than 0
  float vDotH = max(dot(v, h), 0.0);
  float nDotL = max(dot(n, l), 0.0);
  float nDotV = max(dot(n, v), 0.0);

  float D = GGXDistribution(nDotH, mat.roughness); // normal distribution
  float G = GeometrySmith(nDotV, nDotL, mat.roughness); // geometry
  vec3 F = SchlickFresnel(vDotH, mat.color.rgb, mat.metallic); // fresnel

  vec3 ks = F; // specular coefficient
  vec3 kd = vec3(1.0) - ks; // diffuse coefficient
  kd *= 1.0 - mat.metallic;

  vec3 numerator = D * G * F;
  float denominator = 4.0 * nDotL * nDotV + 0.0001; // prevents division by 0

  vec3 specularBRDF = numerator / denominator;
  vec3 diffuseBRDF = kd * mat.color.rgb / PI;

  return (diffuseBRDF + specularBRDF) * radiance * nDotL;
}

// Simple global directional light (hardcoded for quick testing)
vec3 CalculatePBRDirectional(DirectionalLightUniforms light, MaterialSample mat, vec3 fragN, vec3 fragP, vec3 camP)
{
  // Direction points from light toward the scene; light comes FROM -dir
  vec3 l = normalize(-light.direction);
  vec3 v = normalize(camP - fragP);
  vec3 h = normalize(v + l);
  vec3 n = normalize(fragN);

  float nDotH = max(dot(n, h), 0.0);
  float vDotH = max(dot(v, h), 0.0);
  float nDotL = max(dot(n, l), 0.0);
  float nDotV = max(dot(n, v), 0.0);

  float D = GGXDistribution(nDotH, mat.roughness); // normal distribution
  float G = GeometrySmith(nDotV, nDotL, mat.roughness); // geometry
  vec3 F = SchlickFresnel(vDotH, mat.color.rgb, mat.metallic); // fresnel

  vec3 ks = F;
  vec3 kd = (vec3(1.0) - ks) * (1.0 - mat.metallic);

  vec3 numerator = D * G * F;
  float denominator = 4.0 * nDotL * nDotV + 0.0001;

  vec3 specularBRDF = numerator / denominator;
  vec3 diffuseBRDF = kd * mat.color.rgb / PI;

  vec3 radiance = light.color * light.intensity; // no attenuation for directional

  return (diffuseBRDF + specularBRDF) * radiance * nDotL;
}