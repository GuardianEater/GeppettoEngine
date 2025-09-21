#version 460
#extension GL_ARB_bindless_texture : require

#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
in vec4 worldNormal;   // the normal vector of the surface hit
in vec4 worldPosition; // the point that the light hits
in vec2 uvOut;       // the uv coordinates of the surface hit
flat in int vObjectIndex;

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

// forward /////////////////////////////////////////////////////////////////////
vec3 CalculatePBRLightingTotal();

void main(void)
{
  const ObjectUniforms object = objectUniforms[vObjectIndex];

  // Normalize surface normal and compute view vector.
  vec3 N = normalize(worldNormal.xyz);
  
  // Determine the base color from texture or solid color.
  if (object.isSolidColor == 1 || object.isWireframe == 1) 
  {
    frag_color = object.material.color;
    return;
  } 

  if (object.isIgnoringLight == 1)
  {
    if (object.isUsingTexture == 1) 
    {
      frag_color = texture(textureSampler, uvOut);
    }
    else
    {
      frag_color = object.material.color;
    }
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
  F0 = mix(F0, vec3(objectUniforms[vObjectIndex].material.color), objectUniforms[vObjectIndex].material.metallic);

  const float clamped = clamp(1.0 - vDotH, 0.0, 1.0);
  const vec3 result = F0 + (1.0 - F0) * pow(clamped, 5);

  return result;
}

float GeometrySchlickGGX(float dp)
{
  float k = (objectUniforms[vObjectIndex].material.roughness + 1.0) * (objectUniforms[vObjectIndex].material.roughness + 1.0) / 8.0;
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
  float alpha2 = pow(objectUniforms[vObjectIndex].material.roughness, 4);
  float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
  float ggx = alpha2 / (PI * d * d);

  return ggx;
}

// only for point lights
vec3 CalculatePBRLighting(LightUniforms light, vec3 n, vec3 objectColor)
{
  vec3 l = vec3(0.0);

  l = light.position - worldPosition.xyz;
  float lightToPixelDist = length(l); // before normalizing get the length
  l = normalize(l);
  
  float attenuation = 1.0 / (lightToPixelDist * lightToPixelDist);
  vec3 radiance = light.color * attenuation * light.intensity * 100.0;

  vec3 v = normalize(cameraUniforms[cameraIndex].camPosition.xyz - worldPosition.xyz); // view vector
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
  kd *= 1.0 - objectUniforms[vObjectIndex].material.metallic;

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
  vec3 objectColor = vec3(objectUniforms[vObjectIndex].material.color);
  if (objectUniforms[vObjectIndex].isUsingTexture == 1) 
  {
    objectColor *= texture(textureSampler, uvOut).rgb;
  }

  for (int i = 0; i < lightCount; i++) 
  {
    color += CalculatePBRLighting(lights[i], n, objectColor);
  }

  vec3 ambient = vec3(0.8) * objectColor * objectUniforms[vObjectIndex].material.ao;
  color += ambient;

  // HDR tone mapping
  //color /= (color + vec3(1.0));

  //gamma correction
  //color = pow(color, vec3(1.0 / 2));

  return color;
}