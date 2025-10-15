/*//////////////////////////////////////////////////////////////////////////////
 * author : 2018t
 * date   : May 2025
 *
 * brief  : a common file where all shared shader information is defined
/*//////////////////////////////////////////////////////////////////////////////

// start ///////////////////////////////////////////////////////////////////////
#version 460
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

// structures //////////////////////////////////////////////////////////////////

struct LightUniforms
{
  vec3 position;   // the location of the light source
  vec3 color;      // the color of the light source
  float intensity; 
};

// the material of the current object
struct PBRMaterial
{
  float ao;       // ambientOcclusion
  float roughness;
  float metallic; 
  float pad;

  vec4 color;     // diffuse

  uint64_t aoTextureHandle;
  uint64_t roughnessTextureHandle;
  uint64_t metallicTextureHandle;
  uint64_t colorTextureHandle;
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
  mat4 normalMatrix;

  int isUsingTexture;  
  int isIgnoringLight; 
  int isSolidColor;    
  int isWireframe;   
  PBRMaterial material;  

  int boneOffset;
  int pad[3];
};

struct BoneUniforms
{
  mat4 transform;
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

layout(std430, binding = 4) buffer MaterialUniformsBuffer 
{
  PBRMaterial materialUniforms[];
};

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform int cameraIndex;   // the currently active camera in the cameraUniforms array
//layout(location=1) used by color in the line shader
layout(location=2) uniform int lightCount;    // the total amount of lights in the lights array

uniform sampler2D textureSampler;

// constants ///////////////////////////////////////////////////////////////////
const float PI = 3.14159265359;
