#version 430

// structures //////////////////////////////////////////////////////////////////

struct Light
{
  vec3 position; // the location of the light source
  vec3 color; // the color of the light source
  float intensity;
};

struct PBRMaterial
{
  float ao; // ambientOcclusion
  float roughness;
  float metallic;
  vec3 color; // albedo
};

// in variables ////////////////////////////////////////////////////////////////
in vec4 worldNormal;   // the normal vector of the surface hit
in vec4 worldPosition; // the point that the light hits
in vec2 uvOut;       // the uv coordinates of the surface hit

// uniforms ////////////////////////////////////////////////////////////////////
uniform vec4 camPosition;
uniform sampler2D textureSampler;
uniform int isUsingTexture;
uniform int isIgnoringLight;
uniform int isSolidColor;
uniform int isHighlighted;
uniform int lightCount;
uniform vec3 solidColor; // the solid color to use, when isSolidColor is true
uniform PBRMaterial material;

// constants ///////////////////////////////////////////////////////////////////
const float PI = 3.14159265359;

layout(std430, binding=1) buffer LightBuffer
{
  Light lights[];
};

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

// forward /////////////////////////////////////////////////////////////////////
vec3 CalculatePBRLightingTotal();

void main(void)
{
  // Normalize surface normal and compute view vector.
  vec3 N = normalize(worldNormal.xyz);
  
  // Determine the base color from texture or solid color.
  vec3 baseColor = vec3(1.0);
  if (isSolidColor == 1) 
  {
    frag_color = vec4(solidColor, 1.0);
    return;
  } 
  else if (isHighlighted == 1)
  {
    frag_color = vec4(1.0, 1.0, 0.0, 1.0);
    return;
  }
  else
  {
    baseColor = material.color;
  }

  if (isIgnoringLight == 1)
  {
    frag_color = vec4(baseColor, 1.0);
    return;
  }
  
  vec3 finalColor = CalculatePBRLightingTotal();

  frag_color = vec4(finalColor, 1.0);
}

// calculates F in the pbr equation
vec3 SchlickFresnel(float vDotH)
{
  const float dielectricDefault = 0.04;
  vec3 F0 = vec3(dielectricDefault);
  F0 = mix(F0, material.color, material.metallic);

  const float clamped = clamp(1.0 - vDotH, 0.0, 1.0);
  const vec3 result = F0 + (1.0 - F0) * pow(clamped, 5);

  return result;
}

float GeometrySchlickGGX(float dp)
{
  float k = (material.roughness + 1.0) * (material.roughness + 1.0) / 8.0;
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
  float alpha2 = pow(material.roughness, 4);
  float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
  float ggx = alpha2 / (PI * d * d);

  return ggx;
}

// only for point lights
vec3 CalculatePBRLighting(Light light, vec3 n, vec3 objectColor)
{
  vec3 l = vec3(0.0);

  l = light.position - worldPosition.xyz;
  float lightToPixelDist = length(l); // before normalizing get the length
  l = normalize(l);
  
  float attenuation = 1.0 / (lightToPixelDist * lightToPixelDist);
  vec3 radiance = light.color * attenuation * light.intensity;

  vec3 v = normalize(camPosition.xyz - worldPosition.xyz); // view vector
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
  kd *= 1.0 - material.metallic;

  vec3 numerator = D * G * F;
  float denominator = 4.0 * nDotL * nDotV + 0.0001; // prevents division by 0

  vec3 specularBRDF = numerator / denominator;
  vec3 diffuseBRDF = kd * objectColor / PI;

  vec3 finalColor = (diffuseBRDF + specularBRDF) * radiance * nDotL;

  return finalColor;
}

vec3 CalculatePBRLightingTotal()
{
  vec3 n = normalize(worldNormal.xyz);
  vec3 color = vec3(0.0); // the final color of the fragment

  // loop over each light.
  vec3 objectColor = material.color;
  if (isUsingTexture == 1) 
  {
    //objectColor = texture(textureSampler, uvOut).rgb;
  }

  for (int i = 0; i < lightCount; i++) 
  {
    color += CalculatePBRLighting(lights[i], n, objectColor);
  }
  vec3 ambient = vec3(0.03) * objectColor * material.ao;
  color += ambient;

  // HDR tone mapping
  color /= (color + vec3(1.0));

  // gamma correction
  color = pow(color, vec3(1.0 / 2.2));

  return color;
}