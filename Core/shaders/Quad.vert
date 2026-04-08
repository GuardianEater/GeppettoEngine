#version 460

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec2 v_uv;
layout(location=1) flat out uint v_InstanceID;

void main()
{
	const vec2 kPositions[3] = vec2[](
		vec2(-1.0, -1.0),
		vec2(3.0, -1.0),
		vec2(-1.0, 3.0)
	);

  v_InstanceID = gl_InstanceID;
	v_uv = kPositions[gl_VertexID] * 0.5 + 0.5;
	gl_Position = vec4(kPositions[gl_VertexID], 0.0, 1.0);
}
