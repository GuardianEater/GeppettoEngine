#include "Common.glsl"

// outs ////////////////////////////////////////////////////////////////////////
layout(location=0) flat out uint v_InstanceID;

// deferred triangle
const vec2 c_verts[3] = {
  vec2(-1.0, -1.0),
  vec2( 3.0, -1.0),
  vec2(-1.0,  3.0)
};

void main() 
{
  gl_Position = vec4(c_verts[gl_VertexID], 0.0, 1.0);
  v_InstanceID = gl_InstanceID;
}