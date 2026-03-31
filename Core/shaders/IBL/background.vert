#include "../Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 a_position;    // position in world space

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec3 v_position;    // position in world space

void main()
{
  v_position = a_position;

	mat4 rotView = mat4(mat3(u_cams[u_camIndex].view));
	vec4 clipPos = u_cams[u_camIndex].perspective * rotView * vec4(v_position, 1.0);

	gl_Position = clipPos.xyww;
}