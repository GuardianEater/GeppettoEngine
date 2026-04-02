#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 v_position;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
uniform sampler2D u_equirectangularMap;

// constants ///////////////////////////////////////////////////////////////////
const vec2 c_invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= c_invAtan;
  uv += 0.5;
  return uv;
}

void main()
{
  vec2 uv = SampleSphericalMap(normalize(v_position));
  vec3 color = texture(u_equirectangularMap, uv).rgb;
  
  f_color = vec4(color, 1.0);
}