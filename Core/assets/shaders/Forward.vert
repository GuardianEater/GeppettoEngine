#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 position;    // surface point

void main()
{
  gl_Position = position;
}