#version 460

// uniform /////////////////////////////////////////////////////////////////////
layout(location=0) uniform float u_exposure;

layout(binding=0) uniform sampler2D u_sceneTexture; // HDR scene texture

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_sceneTexture, 0));
	vec3 hdrColor = texture(u_sceneTexture, uv).rgb * u_exposure;
	
	// reinhard tonemap operator
	vec3 ldrColor = hdrColor / (hdrColor + vec3(1.0));
	
	// gamma correction
	ldrColor = pow(ldrColor, vec3(1.0 / 2.2));
	
	f_color = vec4(ldrColor, 1.0);
}
