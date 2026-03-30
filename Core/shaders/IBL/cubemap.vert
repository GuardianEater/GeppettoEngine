#include "../Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in model space

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec3 v_position;    // position in model space

// uniform /////////////////////////////////////////////////////////////////////
layout(location=4) out mat4 u_capturePV;    // position in model space

void main()
{
  v_position = a_position;
  gl_Position = u_capturePV * vec4(v_position, 1.0);
}