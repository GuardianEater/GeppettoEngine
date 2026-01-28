#include "Common.glsl"

// uniforms ////////////////////////////////////////////////////////////////////
uniform sampler2D u_depthTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_colorTexture;
uniform sampler2D u_armTexture;

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color; // the resulting pixel color

vec3 CalculatePBRLightingTotal();
// forward /////////////////////////////////////////////////////////////////////

// all of the samples in a single struct
struct CurrentSample
{
  float ao;       // ambientOcclusion
  float roughness;
  float metallic; 
  vec4 color;
};

CurrentSample g_currentSample;
vec3 g_normal;
vec3 g_position;

vec3 GetPosition(float depth)
{
  vec2 ndcXY = v_uv * 2.0 - 1.0;
  float ndcZ = depth * 2.0 - 1.0;
  vec4 clip = vec4(ndcXY, ndcZ, 1.0);
  vec4 world = u_cams[u_camIndex].ipvMatrix * clip;
  world /= world.w;

  return world.xyz;
}

void main(void)
{
  float depth = texture(u_depthTexture, v_uv).x;  
  vec3 arm    = texture(u_armTexture, v_uv).xyz;

  g_normal = texture(u_normalTexture, v_uv).xyz;
  g_position = GetPosition(depth);
  g_currentSample.color     = texture(u_colorTexture, v_uv);
  g_currentSample.ao        = arm.x;
  g_currentSample.roughness = arm.y;
  g_currentSample.metallic  = arm.z;

  // compute pbr
  vec3 finalColor = CalculatePBRLightingTotal();

  // Output with alpha for blending
  f_color = vec4(finalColor, g_currentSample.color.a);
}

// calculates F in the pbr equation
vec3 SchlickFresnel(float vDotH)
{
  const float dielectricDefault = 0.04;
  vec3 F0 = vec3(dielectricDefault);
  F0 = mix(F0, vec3(g_currentSample.color), g_currentSample.metallic);

  const float clamped = clamp(1.0 - vDotH, 0.0, 1.0);
  const vec3 result = F0 + (1.0 - F0) * pow(clamped, 5);

  return result;
}

float GeometrySchlickGGX(float dp)
{
  float k = (g_currentSample.roughness + 1.0) * (g_currentSample.roughness + 1.0) / 8.0;
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
  float alpha2 = pow(g_currentSample.roughness, 4);
  float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
  float ggx = alpha2 / (PI * d * d);

  return ggx;
}

// only for point lights
vec3 CalculatePBRPoint(PointLightUniforms light, vec3 n, vec3 objectColor)
{
  vec3 l = vec3(0.0);

  l = light.position - g_position;
  float lightToPixelDist = length(l); // before normalizing get the length
  l = normalize(l);
  
  float attenuation = 1.0 / (lightToPixelDist * lightToPixelDist);
  attenuation = min(attenuation, 0.001); // caps attenuation so it doesnt get super bright to fast
  vec3 radiance = light.color * attenuation * light.intensity;

  vec3 v = normalize(u_cams[u_camIndex].position.xyz - g_position); // view vector
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
  kd *= 1.0 - g_currentSample.metallic;

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
  vec3 v = normalize(u_cams[u_camIndex].position.xyz - g_position);
  vec3 h = normalize(v + l);

  float nDotH = max(dot(n, h), 0.0);
  float vDotH = max(dot(v, h), 0.0);
  float nDotL = max(dot(n, l), 0.0);
  float nDotV = max(dot(n, v), 0.0);

  float D = GGXDistribution(nDotH);
  float G = GeometrySmith(nDotV, nDotL);
  vec3 F = SchlickFresnel(vDotH);

  vec3 ks = F;
  vec3 kd = (vec3(1.0) - ks) * (1.0 - g_currentSample.metallic);

  vec3 numerator = D * G * F;
  float denominator = 4.0 * nDotL * nDotV + 0.0001;

  vec3 specularBRDF = numerator / denominator;
  vec3 diffuseBRDF = kd * objectColor / PI;

  vec3 radiance = lightColor * intensity; // no attenuation for directional

  return (diffuseBRDF + specularBRDF) * radiance * nDotL;
}

vec3 CalculatePBRLightingTotal()
{    
  vec3 n = g_normal;
  vec3 color = vec3(0.0); // the final color of the fragment

  // point lights
  for (int i = 0; i < u_pointLightCount; i++) 
  {
    color += CalculatePBRPoint(u_pointLights[i], n, g_currentSample.color.rgb);
  }

  // directional lights
  for (int i = 0; i < u_directionalLightCount; i++) 
  {
    color += CalculatePBRDirectional(u_directionalLights[i], n, g_currentSample.color.rgb);
  }

  // ambient lighting
  float ambientFactor = 0.1;
  vec3 ambient = vec3(ambientFactor) * g_currentSample.color.rgb * g_currentSample.ao;
  color += ambient;

  // HDR tone mapping
  //color /= (color + vec3(1.0));

  //gamma correction
  //color = pow(color, vec3(1.0 / 2));

  return color;
}