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
layout(location=3) flat out int vObjectIndex; // the current objects index into object uniforms

void main(void)
{
  vObjectIndex = gl_InstanceID + gl_BaseInstance;

  // ---- skin the vertex using bone matrices ----
  vec4 totalPosition = vec4(0.0);
  vec3 totalNormal = vec3(0.0);

  for (int i = 0; i < 4; i++) 
  {
    uint boneIndex = objectUniforms[vObjectIndex].boneOffset + boneIndexs[i];
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