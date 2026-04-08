#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform vec4 u_outlineColor;

layout(binding=0) uniform sampler2D u_outlineMask;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

void main()
{
  float outlineMask = texture(u_outlineMask, v_uv).r;

  f_color = vec4(u_outlineColor.rgb, u_outlineColor.a * outlineMask);
}
