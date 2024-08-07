/************************************************************
 * @file   PhongRender.cpp
 * @author Travis Gronvold (travis.gronvold@digipen.edu)
 * @date   03-12-2024
 * 
 * @brief  Implementation for rendering with the PhongModel
 ************************************************************/

#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 world_normal;   // the normal vector of the surface hit
layout(location=1) in vec4 world_position; // the point that the light hits

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=4) uniform vec4 eye_position;
layout(location=5) uniform vec3 diffuse_coefficient;  
layout(location=6) uniform vec3 specular_coefficient; 
layout(location=7) uniform float specular_exponent;   
layout(location=8) uniform vec3 ambient_color;     

// amount of lights, make sure to recalculate the light locations and change this on CPU side, to change value
const int light_limit = 8;

// light
// for some reason this cant be done in a macro, do it by hand. 9 + (light_limit * #)
layout(location=9) uniform vec4 light_positions[light_limit]; // stores the position of all light
layout(location=17) uniform vec3 light_colors[light_limit];    // stores the color of all lights
layout(location=25) uniform int light_uses[light_limit];       // stores whether the light is active or not

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

void main(void)
{
  frag_color = vec4(1, 0, 0, 0);
}