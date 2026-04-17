// based on https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#version 460

// uniforms ////////////////////////////////////////////////////////////////////
layout(binding=0) uniform sampler2D u_srcTexture;
layout(location=0) uniform bool u_useKarisAverage;

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout (location=0) out vec3 f_color;

vec3 PowVec3(vec3 v, float p)
{
  return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

vec3 ToSRGB(vec3 v)
{ 
  const float invGamma = 1.0 / 2.2;
  return PowVec3(v, invGamma); 
}

float sRGBToLuma(vec3 col)
{
  //return dot(col, vec3(0.2126f, 0.7152f, 0.0722f));
	return dot(col, vec3(0.299, 0.587, 0.114));
}

float KarisAverage(vec3 col)
{
	// Formula is 1 / (1 + luma)
	float luma = sRGBToLuma(ToSRGB(col)) * 0.25;
	return 1.0 / (1.0 + luma);
}


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

	// should only use this on the full size texture not on mips
  if (u_useKarisAverage)
  {
	  vec3 g0 = (a + b + d + e) * (0.125 / 4.0);
	  vec3 g1 = (b + c + e + f) * (0.125 / 4.0);
	  vec3 g2 = (d + e + g + h) * (0.125 / 4.0);
	  vec3 g3 = (e + f + h + i) * (0.125 / 4.0);
	  vec3 g4 = (j + k + l + m) * (0.5 / 4.0);
	  g0 *= KarisAverage(g0);
	  g1 *= KarisAverage(g1);
	  g2 *= KarisAverage(g2);
	  g3 *= KarisAverage(g3);
	  g4 *= KarisAverage(g4);
	  f_color = g0 + g1 + g2 + g3 + g4;
	  f_color = max(f_color, 0.0001);
  }
  else
  {
    // apply weighted distribution on mips
    f_color = e * 0.125;
    f_color += (a + c + g + i) * 0.03125;
    f_color += (b + d + f + h) * 0.0625;
    f_color += (j + k + l + m) * 0.125;
  }
}