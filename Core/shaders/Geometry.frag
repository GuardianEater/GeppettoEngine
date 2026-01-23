#include "Common.glsl"

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) in vec3 worldPosition;       // the point that the light hits
layout(location=1) in vec3 worldNormal;         // the normal vector of the surface hit
layout(location=2) in vec2 uvOut;               // the uv coordinates of the surface hit
layout(location=3) flat in uint vMaterialIndex; // the current material index into material uniforms

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 fWorldPosition;
layout(location=1) out vec4 fWorldNormal;
layout(location=2) out vec4 color;
layout(location=3) out vec4 ao_rough_metal;

void main()
{
  const PBRMaterial mat = materialUniforms[vMaterialIndex];

  ao_rough_metal.x = mat.aoTextureHandle        == uvec2(0,0) ? mat.ao        : texture(sampler2D(mat.aoTextureHandle), uvOut).r;
  ao_rough_metal.y = mat.roughnessTextureHandle == uvec2(0,0) ? mat.roughness : texture(sampler2D(mat.roughnessTextureHandle), uvOut).r;
  ao_rough_metal.z = mat.metallicTextureHandle  == uvec2(0,0) ? mat.metallic  : texture(sampler2D(mat.metallicTextureHandle), uvOut).r;

  color            = mat.colorTextureHandle     == uvec2(0,0) ? mat.color     : texture(sampler2D(mat.colorTextureHandle), uvOut);

  fWorldPosition = vec4(worldPosition, 1.0);
  fWorldNormal = vec4(worldNormal, 0.0);
}