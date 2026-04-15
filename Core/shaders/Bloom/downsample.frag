// based on https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_srcTexture;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location=0) out vec3 f_color;


void main()
{
  const vec2 srcTexelSize = 1.0 / vec2(textureSize(u_srcTexture, 0));
  const float x = srcTexelSize.x;
  const float y = srcTexelSize.y;

  // take 13 samples around current texel:
  const vec3 a = texture(u_srcTexture, vec2(v_uv.x - 2*x, v_uv.y + 2*y)).rgb;
  const vec3 b = texture(u_srcTexture, vec2(v_uv.x,       v_uv.y + 2*y)).rgb;
  const vec3 c = texture(u_srcTexture, vec2(v_uv.x + 2*x, v_uv.y + 2*y)).rgb;
 
  const vec3 d = texture(u_srcTexture, vec2(v_uv.x - 2*x, v_uv.y)).rgb;
  const vec3 e = texture(u_srcTexture, vec2(v_uv.x,       v_uv.y)).rgb; // current
  const vec3 f = texture(u_srcTexture, vec2(v_uv.x + 2*x, v_uv.y)).rgb;
   
  const vec3 g = texture(u_srcTexture, vec2(v_uv.x - 2*x, v_uv.y - 2*y)).rgb;
  const vec3 h = texture(u_srcTexture, vec2(v_uv.x,       v_uv.y - 2*y)).rgb;
  const vec3 i = texture(u_srcTexture, vec2(v_uv.x + 2*x, v_uv.y - 2*y)).rgb;
 
  const vec3 j = texture(u_srcTexture, vec2(v_uv.x - x, v_uv.y + y)).rgb;
  const vec3 k = texture(u_srcTexture, vec2(v_uv.x + x, v_uv.y + y)).rgb;
  const vec3 l = texture(u_srcTexture, vec2(v_uv.x - x, v_uv.y - y)).rgb;
  const vec3 m = texture(u_srcTexture, vec2(v_uv.x + x, v_uv.y - y)).rgb;

  // apply weighted distribution:
  f_color = e * 0.125;
  f_color += (a + c + g + i) * 0.03125;
  f_color += (b + d + f + h) * 0.0625;
  f_color += (j + k + l + m) * 0.125;
}