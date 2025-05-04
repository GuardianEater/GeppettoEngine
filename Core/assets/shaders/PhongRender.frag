/************************************************************
 * @file   PhongRender.cpp
 * @author Travis Gronvold (2018tcg@gmail.com)
 * @date   03-12-2024
 * 
 * @brief  Implementation for rendering with the PhongModel
 ************************************************************/

#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 world_normal;   // the normal vector of the surface hit
layout(location=1) in vec4 world_position; // the point that the light hits
layout(location=2) in vec2 uv;             // the uv coordinates of the surface hit

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=4) uniform vec4 eye_position;
layout(location=5) uniform vec3 diffuse_coefficient;  
layout(location=6) uniform vec3 specular_coefficient; 
layout(location=7) uniform float specular_exponent;   
layout(location=8) uniform vec3 ambient_color;     
layout(location=9) uniform sampler2D texture_sampler;
layout(location=10) uniform bool use_texture;

// amount of lights, make sure to recalculate the light locations and change this on CPU side, to change value
const int light_limit = 8;

// light
// for some reason this cant be done in a macro, do it by hand. 9 + (light_limit * #)
layout(location=11) uniform vec4 light_positions[light_limit]; // stores the position of all light
layout(location=19) uniform vec3 light_colors[light_limit];    // stores the color of all lights
layout(location=27) uniform int light_uses[light_limit];       // stores whether the light is active or not

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

void main(void)
{
  // this is the total amount of light on a point
  vec3 light_total = {0, 0, 0};

  for (int i = 0; i < light_limit; i++)
  {
    // if the light is not in use dont do anything
    if (light_uses[i] != 1) continue;

    // light vector
    vec4 L = light_positions[i] - world_position;
    L = normalize(L);

    // m dot L
    float mdl = dot(world_normal, L);
    vec3 specular = {0, 0, 0};
    vec3 diffuse = {0, 0, 0};

    // if mdl < 0 then there is no need to do any of these calculations
    if (mdl > 0.0f) 
    {
      // direction of specular reflection, no need to normalize
      vec4 Rl = ((2 * mdl) * world_normal) - L;

      // viewing vector
      vec4 V = eye_position - world_position;
      V = normalize(V);

      // Rl dot V
      float RldV = dot(Rl, V);
      if (RldV < 0.0f) RldV = 0.0f;

      // calculates the specular part
      specular = specular_coefficient * pow(RldV, specular_exponent) * light_colors[i];

      // calculates the diffuse part
      diffuse = diffuse_coefficient * mdl * light_colors[i];

      // adds up the all the light from all the lights
      light_total += specular + diffuse;
    }
  }

  // calculates the ambient part
  vec3 ambient = diffuse_coefficient * ambient_color;

  // adds the ambient light to the light total
  light_total += ambient;

  vec3 surface_color = use_texture 
                     ? texture(texture_sampler, uv).rgb 
                     : vec3(1, 1, 1);

  vec3 final_color = surface_color * light_total;

  // calculates the total color
  frag_color = vec4(final_color, 1.0f);
}