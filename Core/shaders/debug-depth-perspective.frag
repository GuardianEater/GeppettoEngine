#version 460

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
layout(location=0) uniform sampler2D u_depthTexture;
layout(location=1) uniform float u_near;
layout(location=2) uniform float u_far;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
  float z = depth * 2.0 - 1.0; // Back to NDC 
  return (2.0 * u_near * u_far) / (u_far + u_near - z * (u_far - u_near));	
}

void main()
{             
  float depthValue = texture(u_depthTexture, v_uv).r;
  f_color = vec4(vec3(LinearizeDepth(depthValue) / u_far), 1.0); // perspective
}