#version 430

#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
in vec4 worldNormal;   // the normal vector of the surface hit
in vec4 worldPosition; // the point that the light hits
in vec2 uvOut;       // the uv coordinates of the surface hit

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

void main(void)
{
  frag_color = vec4(1.0, 1.0, 0.0, 1.0);
}