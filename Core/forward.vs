#version 430

/*  Vertex data */
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 nrm;
layout (location = 2) in vec2 uv;

layout (location = 3) uniform mat4 mvMat;
layout (location = 4) uniform mat4 normalMat;
layout (location = 5) uniform mat4 projMat;

layout (location = 8) uniform int  numLights;
layout (location = 9) uniform vec3 lightPosVF[10];  /*	Some drivers only support these many elements */


out vec2 uvCoord;

/*  These output vectors are in view frame */
out vec3 lightVF[10];           /*  Light vector from vertex to light source */
out vec3 nrmVF;                 /*  Vertex normal */
out vec3 viewVF;                /*  View normal from vertex to camera */


void main(void) 
{
    /*  Vertex position in view frame */
    vec4 posVF = mvMat * vec4(pos, 1.0);

    /*	For lighting: computing normal, view and light vectors in view frame */

    nrmVF = normalize((normalMat * vec4(nrm, 1.0f)).xyz);

    viewVF = -normalize(posVF.xyz);

    for (int i = 0; i < numLights; i++)
    {
        lightVF[i] = normalize(lightPosVF[i] - posVF.xyz);
    }



	/*	For object transformation */
    gl_Position = projMat * posVF;

    /*  For object texturing */
    uvCoord = uv;
}