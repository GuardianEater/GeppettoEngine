// based on https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=0) uniform float u_filterRadius; // in texture coordinates
layout(binding=0) uniform sampler2D u_srcTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location=0) out vec3 f_color;

void main()
{
  const float x = u_filterRadius;
  const float y = u_filterRadius;

  // 8 samples around current texel:
  const vec3 a = texture(u_srcTexture, vec2(v_uv.x - x, v_uv.y + y)).rgb;
  const vec3 b = texture(u_srcTexture, vec2(v_uv.x,     v_uv.y + y)).rgb;
  const vec3 c = texture(u_srcTexture, vec2(v_uv.x + x, v_uv.y + y)).rgb;
 
  const vec3 d = texture(u_srcTexture, vec2(v_uv.x - x, v_uv.y)).rgb;
  const vec3 e = texture(u_srcTexture, vec2(v_uv.x,     v_uv.y)).rgb; // current
  const vec3 f = texture(u_srcTexture, vec2(v_uv.x + x, v_uv.y)).rgb;
 
  const vec3 g = texture(u_srcTexture, vec2(v_uv.x - x, v_uv.y - y)).rgb;
  const vec3 h = texture(u_srcTexture, vec2(v_uv.x,     v_uv.y - y)).rgb;
  const vec3 i = texture(u_srcTexture, vec2(v_uv.x + x, v_uv.y - y)).rgb;

  // weighted distribution using a 3x3 tent filter
  f_color = e * 4.0;
  f_color += (b + d + f + h) * 2.0;
  f_color += (a + c + g + i);
  f_color *= 1.0 / 16.0;
}