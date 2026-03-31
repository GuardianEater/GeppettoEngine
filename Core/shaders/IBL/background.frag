#include "../Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 v_position;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
uniform samplerCube u_environmentMap;

void main()
{
  vec3 envColor = texture(u_environmentMap, v_position).rgb;
  
  // HDR tonemap and gamma correct
  envColor = envColor / (envColor + vec3(1.0));
  envColor = pow(envColor, vec3(1.0/2.2)); 
  
  f_color = vec4(envColor, 1.0);
}