/// Shader used to compute point light shadows. outputs to a depth map

#include "Common.glsl"

// geometry ////////////////////////////////////////////////////////////////////
layout(triangles) in;
layout(triangle_strip, max_vertices=18) out;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 g_fragPos; // fragment position output per emit vertex

// uniform /////////////////////////////////////////////////////////////////////
layout(location=2) uniform uint u_lightIndex;

void main()
{
  for (int face = 0; face < 6; ++face)
  {
    gl_Layer = face;
    for (int i = 0; i < 3; ++i)
    {
      g_fragPos = gl_in[i].gl_Position;
      gl_Position = u_pointLightShadows[u_lightIndex].shadowMatrices[face] * g_fragPos;
      EmitVertex();
    }
    EndPrimitive();
  }
}