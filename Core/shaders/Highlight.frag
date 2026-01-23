#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 worldPosition; // the point that the light hits
layout(location=1) in vec3 worldNormal;   // the normal vector of the surface hit
layout(location=2) in vec2 uvOut;       // the uv coordinates of the surface hit

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

void main(void)
{
  frag_color = vec4(1.0, 1.0, 0.0, 1.0);
}