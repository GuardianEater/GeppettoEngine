#version 460

// uniform /////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_ssaoTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out float f_color;

void main() 
{
  const vec2 noiseSize = vec2(4.0, 4.0);

  vec2 texelSize = 1.0 / vec2(textureSize(u_ssaoTexture, 0));
  float result = 0.0;
  for (int x = -2; x < 2; ++x) 
  {
    for (int y = -2; y < 2; ++y) 
    {
      vec2 offset = vec2(float(x), float(y)) * texelSize;
      result += texture(u_ssaoTexture, v_uv + offset).r;
    }
  }

  f_color = result / (noiseSize.x * noiseSize.y);
}  