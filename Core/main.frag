#version 330 core

uniform sampler2D colorTex;     /*  Base color texture */
uniform sampler2D normalTex;    /*  Normal texture for normal mapping */
uniform sampler2D bumpTex;      /*  Bump texture for bump mapping */

in vec2 uvCoord;

uniform bool lightOn;
uniform int numLights;
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;
uniform int specularPower;

/*  These could be in view space or tangent space */
in vec3 lightDir[10];
in vec3 viewDir;
in vec3 normal;

uniform bool normalMappingOn;   /*  whether normal mapping is on */
uniform bool parallaxMappingOn; /*  whether parallax mapping is on */


out vec4 fragColor;


void main(void)
{
  if (!lightOn)
  {
    fragColor = vec4(texture(colorTex, uvCoord).rgb, 1.0);
    return;
  }


  //@TODO

  vec3 V = normalize(viewDir);
  vec3 N;

  if (!parallaxMappingOn)
  {
    fragColor = vec4(texture(colorTex, uvCoord).rgb, 1.0);
  }

  if (!normalMappingOn)  
  {
    N = normalize(normal);
  }
  else  
  {
    if (!parallaxMappingOn)  
    {
      N = normalize(texture(normalTex, uvCoord).xyz * 2.0 - 1.0);
    }
    else
    {
      float h = texture(bumpTex, uvCoord).r * 0.15 - 0.005;
      vec2 uvCoord2 = uvCoord + h * V.xy;

      fragColor = vec4(texture(colorTex, uvCoord2).rgb, 1.0);
      N = normalize(texture(normalTex, uvCoord2).xyz * 2.0 - 1.0);
    }
  }
  
  vec4 intensity = ambient;

  for (int i = 0; i < numLights; ++i)
  {
    vec3 L = normalize(lightDir[i]);
    vec3 H = normalize(V + L);

    float NL = max(dot(N,L), 0.0f);
    float NH = max(dot(N,H), 0.0f);

    intensity += diffuse * NL + specular * pow(NH, specularPower);
  }

  fragColor = fragColor * intensity;
}