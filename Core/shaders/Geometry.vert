#include "Common.glsl"
#include "VertexUtility.glsl"

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
  uint meshIndex = gl_InstanceID + u_meshBaseInstance;

  Vertex worldVertex = VertexToWorld(a_position, a_normal, a_boneIndexs, a_boneWeights);
  
  v_normal = worldVertex.normal;
  v_uv = a_uv;
  v_matIndex = u_meshes[meshIndex].materialIndex;
  gl_Position = u_cams[u_camIndex].pvMatrix * worldVertex.position;
}