#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 position; // surface point
layout(location=1) in vec4 normal;   // normal at position
layout(location=2) in vec2 uv;       // texture coordinates

// out to fragment shader //////////////////////////////////////////////////////
layout(location=0) out vec4 worldPosition; // surface point
layout(location=1) out vec4 worldNormal;   // normal at position
layout(location=2) out vec2 uvOut;         // texture coordinates

void main()
{
  // Outline thickness in NDC (not pixels). ~0.004 ~= 0.4% of half-screen.
  // Increase/decrease to taste without touching C++.
  const float uOutlineThicknessNDC = 0.01;

  uint objectIndex = gl_InstanceID + gl_BaseInstance;
  ObjectUniforms obj = objectUniforms[objectIndex];
  CameraUniforms cam = cameraUniforms[cameraIndex];

  // World-space
  vec4 wpos = obj.modelMatrix * position;
  vec3 wnrm = normalize(obj.normalMatrix * normal.xyz);

  // View-space
  vec4 vpos = cam.viewMatrix * wpos;
  vec3 vnrm = normalize((cam.viewMatrix * vec4(wnrm, 0.0)).xyz);

  // Project to clip-space first (no extrusion yet)
  vec4 clipPos = cam.perspectiveMatrix * vpos;

  // Compute screen-space direction: project the view-space normal into clip-space XY.
  // This gives a stable 2D direction to slide the vertex on screen.
  vec2 dir = (cam.perspectiveMatrix * vec4(vnrm, 0.0)).xy;
  float dirLen = max(length(dir), 1e-5); // avoid NaNs for near-zero XY normals
  dir /= dirLen;

  // Offset in clip-space so that NDC offset is constant across depth:
  // NDC = clip.xy / clip.w, so adding (offset * clip.w) keeps NDC offset == constant.
  clipPos.xy += dir * (uOutlineThicknessNDC * clipPos.w);

  gl_Position = clipPos;

  // Populate varyings (even if the frag shader doesn’t use them)
  worldPosition = wpos;
  worldNormal   = vec4(wnrm, 0.0);
  uvOut         = uv;
}