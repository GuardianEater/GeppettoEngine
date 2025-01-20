#version 330 core

/*  These vertex attributes are in model space */
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 nrm;
layout (location = 2) in vec3 tan;
layout (location = 3) in vec3 bitan;
layout (location = 4) in vec2 uv;

uniform mat4 mvMat;     /*  model-view matrix for positions */
uniform mat4 nmvMat;    /*  model-view matrix for normals */
uniform mat4 projMat;   /*  projection matrix */

uniform bool lightOn;           /*  whether lighting should be applied */
uniform int  numLights;
uniform vec3 lightPosVF[10];    /*  light pos already in view frame */

uniform bool normalMappingOn;   /*  whether normal mapping should be applied */


out vec2 uvCoord;

/*  Output vectors:
    - If normalMapping is on then these vectors are in tangent space.
    - Otherwise they are in view space
*/
out vec3 lightDir[10];
out vec3 viewDir;
out vec3 normal;


void main(void) 
{
    vec4 posVF = mvMat * vec4(pos, 1.0);

    /*    For object transformation */
    gl_Position = projMat * posVF;

    /*  For object texturing */
    uvCoord = uv;


    if (lightOn)
    {
        //@TODO                                                                           
        /*  Compute view direction, light direction and normal in view space */           
        viewDir = -posVF.xyz;                           /*  view dir in view frame */     
        normal = normalize(mat3(nmvMat) * nrm);         /*  normal in view frame */       
        for (int i = 0; i < numLights; ++i)                                               
          lightDir[i] = lightPosVF[i] - posVF.xyz;    /*  light dir in view frame */      
                                                                                      
                                                                                      
        if (normalMappingOn)                                                              
        {                                                                                 
          //@TODO                                                                         
          /*  If normal mapping is used, bring view dir and light dir to tangent space.   
          To do this, the TBN matrix needs to be used.                                    
          Normal doesn't need to be transformed since we'll obtain it directly            
          from the normal texture in the fragment shader.                                 
          */                                                                              
                                                                                      
          /*  tangent and bitangent in view frame */                                      
          vec3 tVF = normalize(mat3(mvMat) * tan);                                        
          vec3 bVF = normalize(mat3(mvMat) * bitan);                                      
          vec3 nVF = normal;                                                              
                                                                                      
          mat3 TBNTranspose = transpose(mat3(tVF, bVF, nVF));                             
                                                                                      
          viewDir = TBNTranspose * viewDir;               /*  view dir in tangent frame */
                                                                                      
          for (int i = 0; i < numLights; ++i)                                             
            lightDir[i] = TBNTranspose * lightDir[i];   /*  light dir in tangent frame */ 
        }  
    }
}