#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec4 position;    // surface point
layout(location=1) in vec4 normal;      // normal at position
layout(location=2) in vec2 uv;          // texture coordinates
layout(location=3) in uvec4 boneIndexs; // indices of bones affecting this vertex
layout(location=4) in vec4 boneWeights; // weights of bones affecting this vertex

// out to fragment shader //////////////////////////////////////////////////////
layout(location=0) out vec4 worldPosition;    // surface point
layout(location=1) out vec4 worldNormal;      // normal at position
layout(location=2) out vec2 uvOut;            // texture coordinates
layout(location=3) flat out uint vObjectIndex; // the current objects index into object uniforms
layout(location=4) flat out uint vMeshIndex;   // the current mesh index into mesh uniforms

void main(void)
{
  vObjectIndex = gl_InstanceID + gl_BaseInstance;
  vMeshIndex   = gl_InstanceID + meshBaseInstance;

  worldPosition = objectUniforms[vObjectIndex].modelMatrix * position;

  worldNormal = vec4(normalize(transpose(inverse(mat3(objectUniforms[vObjectIndex].modelMatrix))) * vec3(normal)), 1.0);

  uvOut = uv;

  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * worldPosition;
}