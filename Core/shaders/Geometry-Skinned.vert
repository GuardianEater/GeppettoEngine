#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in model space
layout(location=1) in vec3 a_normal;      // normal in model space
layout(location=2) in vec2 a_uv;          // texture coordinates
layout(location=3) in uvec4 a_boneIndexs; // indices of bones affecting this vertex
layout(location=4) in vec4 a_boneWeights; // weights of bones affecting this vertex

// out to fragment shader //////////////////////////////////////////////////////
layout(location=0) out vec3 v_normal;        // normal in world space
layout(location=1) out vec2 v_uv;            // texture coordinates
layout(location=2) flat out uint v_matIndex; // the current material index into material uniforms

void main(void)
{
  uint objectIndex = gl_InstanceID + gl_BaseInstance;
  uint meshIndex = gl_InstanceID + u_meshBaseInstance;
  vec4 totalPosition = vec4(0.0);
  vec3 totalNormal = vec3(0.0);

  for (int i = 0; i < 4; i++) 
  {
    if (a_boneIndexs[i] == INVALID_INDEX)
      continue; // do nothing if the bone index is not set

    uint boneIndex = u_objects[objectIndex].boneOffset + a_boneIndexs[i];
    if (a_boneWeights[i] > 0.0) 
    {
      vec4 localPosition = u_bones[boneIndex].transform * vec4(a_position, 1.0);
      totalPosition += localPosition * a_boneWeights[i];

      vec3 localNormal = mat3(u_bones[boneIndex].transform) * a_normal;
      totalNormal += localNormal * a_boneWeights[i];
    }
  }

  vec4 pos4 = u_objects[objectIndex].modelMatrix * totalPosition;

  v_normal = normalize(u_objects[objectIndex].normalMatrix * totalNormal);
  v_uv = a_uv;
  v_matIndex = u_meshes[meshIndex].materialIndex;

  gl_Position = u_cams[u_camIndex].pvMatrix * pos4;
}