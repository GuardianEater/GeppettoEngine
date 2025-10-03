#version 460
#extension GL_ARB_bindless_texture : require

#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
in vec4 position; // surface point
in vec4 normal;   // normal at position
in vec2 uv;       // texture coordinates
in uvec4 boneIndexs; // indices of bones affecting this vertex
in vec4 boneWeights;   // weights of bones affecting this vertex

// out to fragment shader //////////////////////////////////////////////////////
out vec4 worldPosition; // surface point
out vec4 worldNormal;   // normal at position
out vec2 uvOut;        // texture coordinates
flat out int vObjectIndex;

void main(void)
{
  vObjectIndex = gl_InstanceID + gl_BaseInstance;

  // ---- skin the vertex using bone matrices ----
  vec4 skinnedPos = vec4(0.0);
  vec3 skinnedNormal = vec3(0.0);

  for (int i = 0; i < 4; i++) 
  {
    uint boneIndex = boneIndexs[i];
    float weight = boneWeights[i];
    if (weight > 0.0) 
    {
      mat4 boneMat = boneUniforms[boneIndex].transform;
      skinnedPos    += weight * (boneMat * position);
      skinnedNormal += weight * mat3(boneMat) * vec3(normal);
    }
  }

  // ---- transform to world space ----
  worldPosition = objectUniforms[vObjectIndex].modelMatrix * skinnedPos;

  // correct normal transform (TBN-safe)
  mat3 normalMatrix = transpose(inverse(mat3(objectUniforms[vObjectIndex].modelMatrix)));
  worldNormal = vec4(normalize(normalMatrix * skinnedNormal), 1.0);

  uvOut = uv;

  // ---- project into clip space ----
  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix *
                cameraUniforms[cameraIndex].viewMatrix *
                worldPosition;
}