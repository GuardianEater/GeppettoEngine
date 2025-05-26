#version 430

// in variables ////////////////////////////////////////////////////////////////
in vec4 position; // surface point
in vec4 normal;   // normal at position
in vec2 uv;       // texture coordinates

// out to fragment shader //////////////////////////////////////////////////////
out vec4 worldPosition; // surface point
out vec4 worldNormal;   // normal at position
out vec2 uvOut;         // texture coordinates

// uniforms ////////////////////////////////////////////////////////////////////
uniform mat4 perspectiveMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 normalMatrix;

void main(void)
{
  worldPosition = modelMatrix * position;

  worldNormal = normalize(normalMatrix * normal);

  uvOut = uv;

  gl_Position = perspectiveMatrix * viewMatrix * worldPosition;
}