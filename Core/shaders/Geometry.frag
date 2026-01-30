#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 v_normal;         // the normal vector of the surface hit
layout(location=1) in vec2 v_uv;               // the uv coordinates of the surface hit
layout(location=2) flat in uint v_matIndex; // the current material index into material uniforms

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec3 f_normal;
layout(location=1) out vec4 f_color;
layout(location=2) out vec3 f_arm;

void main()
{
  const PBRMaterial mat = u_materials[v_matIndex];

  f_arm.x = mat.aoTextureHandle        == uvec2(0,0) ? mat.ao        : texture(sampler2D(mat.aoTextureHandle), v_uv).r;
  f_arm.y = mat.roughnessTextureHandle == uvec2(0,0) ? mat.roughness : texture(sampler2D(mat.roughnessTextureHandle), v_uv).r;
  f_arm.z = mat.metallicTextureHandle  == uvec2(0,0) ? mat.metallic  : texture(sampler2D(mat.metallicTextureHandle), v_uv).r;
  f_color = mat.colorTextureHandle     == uvec2(0,0) ? mat.color     : texture(sampler2D(mat.colorTextureHandle), v_uv);

  f_normal = v_normal;

  // discard alpha if its small
  const float ALPHA_CUTOFF = 0.01;
  if (f_color.a < ALPHA_CUTOFF)
    discard;
}