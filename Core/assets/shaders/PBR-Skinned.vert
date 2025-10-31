#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 position;    // surface point
layout(location=1) in vec3 normal;      // normal at position
layout(location=2) in vec2 uv;          // texture coordinates
layout(location=3) in uvec4 boneIndexs; // indices of bones affecting this vertex
layout(location=4) in vec4 boneWeights; // weights of bones affecting this vertex

// out to fragment shader //////////////////////////////////////////////////////
layout(location=0) out vec3 worldPosition;    // surface point
layout(location=1) out vec3 worldNormal;      // normal at position
layout(location=2) out vec2 uvOut;            // texture coordinates
layout(location=3) flat out uint vObjectIndex; // the current objects index into object uniforms
layout(location=4) flat out uint vMeshIndex;   // the current mesh index into mesh uniforms
layout(location=5) flat out uint vMaterialIndex;   // the current material index into material uniforms

const uint INVALID_INDEX = 4294967295;

void main(void)
{
  vObjectIndex   = gl_InstanceID + gl_BaseInstance;
  vMeshIndex     = gl_InstanceID + meshBaseInstance;
  vMaterialIndex = meshUniforms[vMeshIndex].materialIndex;

  // ---- skin the vertex using bone matrices ----
  vec4 totalPosition = vec4(0.0);
  vec3 totalNormal = vec3(0.0);

  for (int i = 0; i < 4; i++) 
  {
    if (boneIndexs[i] == INVALID_INDEX)
      continue; // do nothing if the bone index is not set

    uint boneIndex = objectUniforms[vObjectIndex].boneOffset + boneIndexs[i];
    float weight = boneWeights[i];
    if (weight > 0.0) 
    {
      vec4 localPosition = boneUniforms[boneIndex].transform * vec4(position, 1.0);
      totalPosition += localPosition * weight;

      vec3 localNormal = mat3(boneUniforms[boneIndex].transform) * normal;
      totalNormal += localNormal * weight;
    }
  }

  // ---- transform to world space ----
  vec4 pos4 = objectUniforms[vObjectIndex].modelMatrix * totalPosition;

  // correct normal transform (TBN-safe)
  mat3 normalMatrix = objectUniforms[vObjectIndex].normalMatrix;

  worldNormal = normalize(normalMatrix * totalNormal);
  worldPosition = vec3(pos4);
  uvOut = uv;

  // ---- project into clip space ----
  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix *
                cameraUniforms[cameraIndex].viewMatrix *
                pos4;
}