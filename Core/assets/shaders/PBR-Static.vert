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
layout(location=3) flat out uint vMaterialIndex;   // the current material index into material uniforms

void main(void)
{
  uint objectIndex   = gl_InstanceID + gl_BaseInstance;
  uint meshIndex     = gl_InstanceID + meshBaseInstance;
  vMaterialIndex = meshUniforms[meshIndex].materialIndex;

  vec4 pos4 = objectUniforms[objectIndex].modelMatrix * vec4(position, 1.0);

  worldNormal = normalize(objectUniforms[objectIndex].normalMatrix * normal);
  uvOut = uv;
  worldPosition = vec3(pos4);

  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * pos4;
}