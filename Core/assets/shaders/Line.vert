#version 460

#include "Common.glsl"

// in 
layout(location=0) in vec3 inPosition;

void main()
{
  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * vec4(inPosition, 1.0);
}