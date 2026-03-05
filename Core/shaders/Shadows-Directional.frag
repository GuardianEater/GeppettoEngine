#include "Common.glsl"

// outs ////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

void main()
{
  float z = gl_FragCoord.z;
  float z2 = z * z;
  float z3 = z2 * z;
  float z4 = z3 * z;

  f_color = vec4(z, z2, z3, z4);
}