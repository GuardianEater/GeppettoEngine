#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 worldPosition;       // the point that the light hits
layout(location=1) in vec3 worldNormal;         // the normal vector of the surface hit
layout(location=2) in vec2 uvOut;               // the uv coordinates of the surface hit
layout(location=3) flat in uint vMaterialIndex; // the current material index into material uniforms

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec3 fWorldPosition;
layout(location=1) out vec3 fWorldNormal;
layout(location=2) flat out uint fMaterialIndex;

void main()
{
  fWorldPosition = worldPosition;
  fWorldNormal = worldNormal;
  fMaterialIndex = materialIndex;
}