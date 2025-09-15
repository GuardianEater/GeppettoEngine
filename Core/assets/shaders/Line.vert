#version 460

// stores all per camera data
struct CameraUniforms
{
  mat4 perspectiveMatrix;
  mat4 viewMatrix;
  
  vec4 camPosition;
};

layout(std430, binding=2) buffer CameraUniformsBuffer
{
  CameraUniforms cameraUniforms[];
};

layout(location=0) uniform int cameraIndex; // the currently active camera in the cameraUniforms array

layout(location=0) in vec4 inPosition;

void main()
{
  gl_Position = cameraUniforms[cameraIndex].perspectiveMatrix * cameraUniforms[cameraIndex].viewMatrix * inPosition;
}