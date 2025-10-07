/*//////////////////////////////////////////////////////////////////////////////
 * author : 2018t
 * date   : May 2025
 *
 * brief  : a common file where all shared shader information is defined
/*//////////////////////////////////////////////////////////////////////////////

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
  vec4 color;     // albedo 
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


// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform int cameraIndex; // the currently active camera in the cameraUniforms array
//layout(location=1) uniform int objectIndex; // the current object in the objectUniforms array
layout(location=2) uniform int lightCount;  // the total amount of lights in the lights array

uniform sampler2D textureSampler;

// constants ///////////////////////////////////////////////////////////////////
const float PI = 3.14159265359;
