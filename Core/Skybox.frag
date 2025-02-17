#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) flat in vec4 world_normal;   // the normal vector of the surface hit
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
layout(location=11) uniform int light_count;
layout(location=12) uniform bool isSolidColor;
layout(location=13) uniform vec3 solidColor;
layout(location=14) uniform bool isHighlighted;

struct Light
{
    vec3 position;
    vec3 color;
    float intensity;
};

layout(std430, binding=1) buffer LightBuffer
{
    Light lights[];
};

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

void main(void)
{
    
}