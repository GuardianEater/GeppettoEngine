#version 460

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
  int objectIndex = gl_InstanceID + gl_BaseInstance;
  worldPosition = objectUniforms[objectIndex].modelMatrix * position;
  gl_Position   = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * worldPosition;
}