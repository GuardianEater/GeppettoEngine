#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 g_fragPos;

// uniform /////////////////////////////////////////////////////////////////////
layout(location=2) uniform uint u_pointLightShadowIndex;

void main()
{
  PointLightShadowUniforms light = u_pointLightShadows[u_pointLightShadowIndex];

  float lightDistance = length(g_fragPos.xyz - light.position);

  lightDistance /= light.farPlane; // normalize

  gl_FragDepth = lightDistance;
}