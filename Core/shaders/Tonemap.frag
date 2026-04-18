#version 460

// uniform /////////////////////////////////////////////////////////////////////
layout(location=0) uniform float u_exposure;

layout(binding=0) uniform sampler2D u_sceneTexture; // HDR scene texture
//layout(binding=1) uniform sampler2D u_bloomTexture; // HDR scene texture

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec2 v_uv;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

void main()
{
  //vec3 bloomColor = texture(u_bloomTexture, v_uv).rgb;
	vec3 hdrColor = texture(u_sceneTexture, v_uv).rgb * u_exposure;// + bloomColor;

	// reinhard tonemap operator
	vec3 ldrColor = hdrColor / (hdrColor + vec3(1.0));
	
	// gamma correction
	ldrColor = pow(ldrColor, vec3(1.0 / 2.2));
	
	f_color = vec4(ldrColor, 1.0);
}
