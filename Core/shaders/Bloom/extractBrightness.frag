#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_hdrTexture;
layout(binding=1) uniform sampler2D u_armeTexture; // anything with emission has bloom reguardless of "brightness"
layout(location=0) uniform float u_threshold = 1.5;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location=0) out vec3 f_color;

void main()
{
  vec3 color = texture(u_hdrTexture, v_uv).rgb;
  float emission = texture(u_armeTexture, v_uv).a;

  float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

  if (brightness > u_threshold || emission > 0.01)
    f_color = color;
  else
    f_color = vec3(0.0);
}