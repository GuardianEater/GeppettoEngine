#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_hdrTexture;
layout(location=0) uniform float u_threshold = 1.5;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location=0) out vec4 f_color;

void main()
{
  vec3 color = texture(u_hdrTexture, v_uv).rgb;

  float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

  if (brightness > u_threshold)
    f_color = vec4(color, 1.0);
  else
    f_color = vec4(0.0);
}