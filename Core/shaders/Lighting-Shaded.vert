#include "Common.glsl"

// ins /////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in model space
layout(location=1) in vec3 a_normal;      // normal in model space
layout(location=2) in vec2 a_uv;          // texture coordinates

// outs ////////////////////////////////////////////////////////////////////////
layout(location=0) flat out uint v_InstanceID;

void main() 
{
  vec4 pos4 = u_pointLightShadows[gl_InstanceID].pointLight.modelMatrix * vec4(a_position, 1.0);
  gl_Position = u_cams[u_camIndex].pvMatrix * pos4;
  v_InstanceID = gl_InstanceID;
}