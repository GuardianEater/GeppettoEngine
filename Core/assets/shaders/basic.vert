/************************************************************
 * @file   PhongRender.cpp
 * @author Travis Gronvold (travis.gronvold@digipen.edu)
 * @date   03-12-2024
 * 
 * @brief  Implementation for rendering with the PhongModel
 ************************************************************/

#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 position; // surface point
layout(location=1) in vec4 normal;   // normal at position

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform mat4 persp_matrix;
layout(location=1) uniform mat4 view_matrix;
layout(location=2) uniform mat4 model_matrix;
layout(location=3) uniform mat4 normal_matrix;

// out to fragment shader //////////////////////////////////////////////////////
layout(location=0) out vec4 world_normal;
layout(location=1) out vec4 world_position;

void main(void)
{
  world_normal = normalize(normal_matrix * normal);
  world_position = model_matrix * position;
  gl_Position = persp_matrix * view_matrix * world_position;
}