#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 g_fragPos;

// uniform /////////////////////////////////////////////////////////////////////
layout(location=2) uniform uint u_lightIndex;

void main()
{
  PointLightShadowUniforms lShadow = u_pointLightShadows[u_lightIndex];
  PointLightUniforms l = lShadow.pointLight;

  float lightDistance = length(g_fragPos.xyz - l.position);

  float cutoff = 0.1;              // chosen threshold
  float farPlane = sqrt(l.intensity / cutoff);

  lightDistance /= farPlane; // normalize

  gl_FragDepth = lightDistance;
}