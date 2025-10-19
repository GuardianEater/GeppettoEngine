/*//////////////////////////////////////////////////////////////////////////////
 * author : 2018t
 * date   : May 2025
 *
 * brief  : a common file where all shared shader information is defined
/*//////////////////////////////////////////////////////////////////////////////

// start ///////////////////////////////////////////////////////////////////////
#version 460
#extension GL_ARB_bindless_texture : require

// structures //////////////////////////////////////////////////////////////////

struct LightUniforms
{
  vec3 position;   // the location of the light source
  float pad;
  vec3 color;      // the color of the light source
  float intensity; 
};

// the material of the current object
struct PBRMaterial
{
  float ao;       // ambientOcclusion
  float roughness;
  float metallic; 
  float pad4;

  vec4 color;     // diffuse

  uvec2 aoTextureHandle;
  uvec2 roughnessTextureHandle;

  uvec2 metallicTextureHandle;
  uvec2 colorTextureHandle;

  uvec2 normalTextureHandle;
  uvec2 pad8;
};

// stores all per camera data
struct CameraUniforms
{
  mat4 perspectiveMatrix;
  mat4 viewMatrix;
  
  vec4 camPosition;
};

// stores all per-object data
struct ObjectUniforms
{
  mat4 modelMatrix;
  mat3 normalMatrix;

  int boneOffset;
  int pad[3];
};

struct BoneUniforms
{
  mat4 transform;
};

struct MeshUniforms
{
  uint materialIndex;

  int pad[3];
};

// buffers /////////////////////////////////////////////////////////////////////
layout(std430, binding=0) buffer ObjectUniformsBuffer
{
  ObjectUniforms objectUniforms[];
};

layout(std430, binding=1) buffer LightUniformsBuffer
{
  LightUniforms lights[];
};

layout(std430, binding=2) buffer CameraUniformsBuffer
{
  CameraUniforms cameraUniforms[];
};

layout(std430, binding=3) buffer BoneUniformsBuffer
{
  BoneUniforms boneUniforms[];
};

layout(std430, binding=4) buffer MaterialUniformsBuffer 
{
  PBRMaterial materialUniforms[];
};

layout(std430, binding=5) buffer MeshUniformsBuffer
{
  MeshUniforms meshUniforms[];
};

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform uint cameraIndex;      // the currently active camera in the cameraUniforms array
//layout(location=1)                             // used by color in the line shader
layout(location=2) uniform uint lightCount;       // the total amount of lights in the lights array
layout(location=3) uniform uint meshBaseInstance; // the base instance of the current mesh, used to index into per mesh data

// constants ///////////////////////////////////////////////////////////////////
const float PI = 3.14159265359;
