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
  vec4 totalPosition = vec4(0.0);
  vec3 totalNormal = vec3(0.0);

  for (int i = 0; i < 4; i++) 
  {
    uint boneIndex = boneIndexs[i];
    float weight = boneWeights[i];
    if (weight > 0.0) 
    {
      vec4 localPosition = boneUniforms[boneIndex].transform * position;
      totalPosition += localPosition * weight;

      vec3 localNormal = mat3(boneUniforms[boneIndex].transform) * normal.xyz;
      totalNormal += localNormal * weight;
    }
  }

  // ---- transform to world space ----
  worldPosition = objectUniforms[vObjectIndex].modelMatrix * totalPosition;

  // correct normal transform (TBN-safe)
  mat3 normalMatrix = transpose(inverse(mat3(objectUniforms[vObjectIndex].modelMatrix)));
  worldNormal = vec4(normalize(normalMatrix * totalNormal), 1.0);

  uvOut = uv;

  // ---- project into clip space ----
  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix *
                cameraUniforms[cameraIndex].viewMatrix *
                worldPosition;
}