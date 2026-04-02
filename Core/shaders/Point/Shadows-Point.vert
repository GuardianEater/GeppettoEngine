#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in model space

void main(void)
{
  uint objectIndex = gl_InstanceID + gl_BaseInstance;
  vec4 pos4 = u_objects[objectIndex].modelMatrix * vec4(a_position, 1.0);

  gl_Position = pos4;
}