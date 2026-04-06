#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform vec2 u_direction;   // (1,0) for horizontal, (0,1) for vertical
layout(location=1) uniform int u_radius;

// textures ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_maskTexture;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_maskTexture, 0));

  float result = 0.0;
  for (int i = -u_radius; i <= u_radius; ++i)
  {
    vec2 offset = (u_direction * float(i)) / vec2(textureSize(u_maskTexture, 0));
    result = max(result, texture(u_maskTexture, uv + offset).r);
  }

  f_color = vec4(result);
}