#include "Common.glsl"

// in //////////////////////////////////////////////////////////////////////////
layout(location=0) in vec3 v_position;

// out /////////////////////////////////////////////////////////////////////////
layout(location=0) out vec4 f_color;

// uniform /////////////////////////////////////////////////////////////////////
uniform samplerCube u_environmentMap;

void main()
{		
  vec3 N = normalize(v_position);

  vec3 irradiance = vec3(0.0);   
  
  // tangent space calculation from origin point
  vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), N));
  vec3 up    = normalize(cross(N, right));
      
  float sampleDelta = 0.025;
  float nrSamples = 0.0f;
  for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
  {
    for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    {
      // spherical to cartesian
      vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));

      // tangent space to world
      vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

      irradiance += texture(u_environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
      nrSamples++;
    }
  }
  irradiance = PI * irradiance * (1.0 / float(nrSamples));
  
  f_color = vec4(irradiance, 1.0);
}
