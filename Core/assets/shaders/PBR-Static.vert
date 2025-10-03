#version 460
#extension GL_ARB_bindless_texture : require

#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
in vec4 position; // surface point
in vec4 normal;   // normal at position
in vec2 uv;       // texture coordinates
in uint boneIndexs[4]; // indices of bones affecting this vertex
in float boneWeights[4];   // weights of bones affecting this vertex

// out to fragment shader //////////////////////////////////////////////////////
out vec4 worldPosition; // surface point
out vec4 worldNormal;   // normal at position
out vec2 uvOut;        // texture coordinates
flat out int vObjectIndex;

void main(void)
{
  vObjectIndex = gl_InstanceID + gl_BaseInstance;

  worldPosition = objectUniforms[vObjectIndex].modelMatrix * position;

  worldNormal = vec4(normalize(transpose(inverse(mat3(objectUniforms[vObjectIndex].modelMatrix))) * vec3(normal)), 1.0);

  uvOut = uv;

  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * worldPosition;
}