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

struct PointLightUniforms
{
  vec3 position;   // the location of the light source
  float pad;
  vec3 color;      // the color of the light source
  float intensity; 

  mat4 modelMatrix; // used for the lights bounding sphere, 
};

// variant of point lights that cast shadows
struct PointLightShadowUniforms
{
  PointLightUniforms pointLight;
  mat4 shadowMatrices[6];
  uvec2 shadowMapHandle;
  uvec2 pad8;
};

struct DirectionalLightUniforms
{
  vec3 position;   // the location of the light source
  float pad;
  vec3 color;      // the color of the light source
  float intensity; 
  vec3 direction;  // the direction the light is pointing
  float pad2;
};

struct DirectionalLightShadowUniforms
{
  DirectionalLightUniforms light;

  mat4 pvMatrix;
  uvec2 shadowMapHandle;
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
  mat4 pvMatrix; // perspective view matrix
  mat4 ipvMatrix; // inverse perspective view matrix
  
  vec3 position;
};

// stores all per-object data
struct ObjectUniforms
{
  mat4 modelMatrix;
  mat3 normalMatrix;

  int boneOffset;
  int pad[3];
};

// stores all per-light-object data
struct LightObjectUniforms
{
  mat4 modelMatrix;
  vec4 color;
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
  ObjectUniforms u_objects[];
};

layout(std430, binding=1) buffer PointLightUniformsBuffer
{
  PointLightUniforms u_pointLights[];
};

layout(std430, binding=2) buffer CameraUniformsBuffer
{
  CameraUniforms u_cams[];
};

layout(std430, binding=3) buffer BoneUniformsBuffer
{
  BoneUniforms u_bones[];
};

layout(std430, binding=4) buffer MaterialUniformsBuffer 
{
  PBRMaterial u_materials[];
};

layout(std430, binding=5) buffer MeshUniformsBuffer
{
  MeshUniforms u_meshes[];
};

layout(std430, binding=6) buffer DirectionalLightUniformsBuffer
{
  DirectionalLightUniforms u_directionalLights[];
};

layout(std430, binding=7) buffer PointLightShadowUniformsBuffer
{
  PointLightShadowUniforms u_pointLightShadows[];
};

layout(std430, binding=8) buffer DirectionalLightShadowUniformsBuffer
{
  DirectionalLightShadowUniforms u_directionalLightShadows[];
};

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform uint u_camIndex;           // the currently active camera in the u_cams array
//layout(location=1)                                   // used by color in the line shader
layout(location=3) uniform uint u_meshBaseInstance;      // the base instance of the current mesh, used to index into per mesh data

// constants ///////////////////////////////////////////////////////////////////
const float PI = 3.14159265359;
const uint INVALID_INDEX = 4294967295;