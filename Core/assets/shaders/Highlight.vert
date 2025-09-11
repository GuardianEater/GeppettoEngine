#version 460

#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
in vec4 position; // surface point
in vec4 normal;   // normal at position
in vec2 uv;       // texture coordinates

// out to fragment shader //////////////////////////////////////////////////////
out vec4 worldPosition; // surface point
out vec4 worldNormal;   // normal at position
out vec2 uvOut;         // texture coordinates

void main()
{
  int objectIndex = gl_InstanceID + gl_BaseInstance;
  worldPosition = objectUniforms[objectIndex].modelMatrix * position;
  gl_Position   = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * worldPosition;
}