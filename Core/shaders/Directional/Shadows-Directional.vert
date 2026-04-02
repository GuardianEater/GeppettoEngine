#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in model space

// uniform /////////////////////////////////////////////////////////////////////
layout(location=2) uniform uint u_lightIndex;

void main()
{
  uint objectIndex = gl_InstanceID + gl_BaseInstance;

  gl_Position = u_directionalLightShadows[u_lightIndex].pvMatrix * u_objects[objectIndex].modelMatrix * vec4(a_position, 1.0);
}