#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 v_position;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
uniform samplerCube u_environmentMap;

void main()
{
  vec3 envColor = texture(u_environmentMap, v_position).rgb;
  //vec3 envColor = textureLod(u_environmentMap, v_position, 3.0).rgb;
  // Keep skybox in linear HDR; final tonemap pass handles tone mapping + gamma once.
  
  f_color = vec4(envColor, 1.0);
}