#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform vec4 u_outlineColor;

layout(binding=0) uniform sampler2D u_outlineMask;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

void main()
{
  vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_outlineMask, 0));
  float outlineMask = texture(u_outlineMask, uv).r;

  f_color = vec4(u_outlineColor.rgb, u_outlineColor.a * outlineMask);
}
