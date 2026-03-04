#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in model space
layout(location=1) in vec3 a_normal;      // normal in model space
layout(location=2) in vec2 a_uv;          // texture coordinates

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec2 v_uv;

void main()
{
  v_uv = a_uv;
  gl_Position = vec4(a_position, 1.0);
}