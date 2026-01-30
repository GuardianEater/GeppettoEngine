#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;

void main()
{
  gl_Position = u_cams[u_camIndex].pvMatrix * vec4(a_position, 1.0);
}