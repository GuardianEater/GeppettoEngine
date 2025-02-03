#version 330 core

/*  This is required for GLSL below 4.3 to explicitly define uniform variable locations */
#extension GL_ARB_explicit_uniform_location : require

/*  Object texture */
layout(location = 7) uniform sampler2D tex;

in vec2 uvCoord;

/*  numLights and light properties */
layout (location = 8) uniform int numLights;
layout (location = 20) uniform vec4 ambient;
layout (location = 21) uniform vec4 diffuse;
layout (location = 22) uniform vec4 specular;
layout (location = 23) uniform int specularPower;

/*  Input vectors in view frame */
in vec3 lightVF[10];
in vec3 nrmVF;
in vec3 viewVF;

out vec4 outColor;

void main(void)
{
  // this is the total amount of light on a point
  vec3 lightTotal = vec3(0.0);

  for (int i = 0; i < numLights; i++)
  {
    // m dot L
    vec3 specularColor = vec3(0.0);
    vec3 diffuseColor = vec3(0.0);
    float mdl = dot(nrmVF, lightVF[i]);

    // if mdl < 0 then there is no need to do any of these calculations
    if (mdl > 0.0f) 
    {
      // direction of specular reflection, no need to normalize
      vec3 Rl = ((2.0f * mdl) * nrmVF) - lightVF[i];

      // Rl dot V
      float RldV = dot(Rl, viewVF);
      if (RldV > 0.0f)
      {
          // calculates the specular part
          specularColor = pow(RldV, specularPower) * specular.rgb;
      }

      diffuseColor = mdl * diffuse.rgb;
      // calculates the diffuse part

      // adds up the all the light from all the lights
      lightTotal += specularColor + diffuseColor;
    }
  }

  vec3 texColor = texture(tex, uvCoord).rgb;

  // adds the ambient light to the light total
  lightTotal += ambient.rgb;
  lightTotal *= texColor;

  // calculates the total color
  outColor = vec4(lightTotal, 1.0f);
  //outColor *= vec4(texture(tex, uvCoord).rgb, 1.0f);
}
