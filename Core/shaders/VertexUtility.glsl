struct Vertex
{
  vec4 position;
  vec3 normal;
};

// does not apply perspective matrix
Vertex VertexToWorld(vec3 position, vec3 normal, uvec4 boneIndices, vec4 boneWeights)
{
  uint objectIndex = gl_InstanceID + gl_BaseInstance;
  Vertex result;
  if (u_rigged)
  {
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);

    for (int i = 0; i < 4; i++) 
    {
      if (boneIndices[i] == INVALID_INDEX)
        continue; // do nothing if the bone index is not set

      uint boneIndex = u_objects[objectIndex].boneOffset + boneIndices[i];
      if (boneWeights[i] > 0.0) 
      {
        vec4 localPosition = u_bones[boneIndex].transform * vec4(position, 1.0);
        totalPosition += localPosition * boneWeights[i];

        vec3 localNormal = mat3(u_bones[boneIndex].transform) * normal;
        totalNormal += localNormal * boneWeights[i];
      }
    }

    result.normal = normalize(u_objects[objectIndex].normalMatrix * totalNormal);
    result.position = u_objects[objectIndex].modelMatrix * totalPosition;
  }
  else
  {
    result.normal = normalize(u_objects[objectIndex].normalMatrix * normal);
    result.position = u_objects[objectIndex].modelMatrix * vec4(position, 1.0);
  }

  return result;
}