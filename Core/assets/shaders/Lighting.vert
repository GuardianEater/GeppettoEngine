#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 position; // surface point
layout(location=1) in vec4 normal;   // normal at position
layout(location=2) in vec2 uv;       // texture coordinates

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform mat4 persp_matrix;
layout(location=1) uniform mat4 view_matrix;
layout(location=2) uniform mat4 model_matrix;
layout(location=3) uniform mat4 normal_matrix;
layout(location=14) uniform bool isHighlighted;

// out to fragment shader //////////////////////////////////////////////////////
layout(location=0) flat out vec4 world_normal;
layout(location=1) out vec4 world_position;
layout(location=2) out vec2 uv_out;

void main(void)
{
    world_position = model_matrix * position;

    world_normal = normalize(normal_matrix * normal);

    uv_out = uv;

    gl_Position = persp_matrix * view_matrix * world_position;
}