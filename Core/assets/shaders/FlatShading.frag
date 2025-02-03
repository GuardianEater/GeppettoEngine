#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) flat in vec4 world_normal;   // the normal vector of the surface hit
layout(location=1) in vec4 world_position; // the point that the light hits
layout(location=2) in vec2 uv;             // the uv coordinates of the surface hit

// uniforms ////////////////////////////////////////////////////////////////////
layout(location=4) uniform vec4 eye_position;
layout(location=5) uniform vec3 diffuse_coefficient;  
layout(location=6) uniform vec3 specular_coefficient; 
layout(location=7) uniform float specular_exponent;   
layout(location=8) uniform vec3 ambient_color;     
layout(location=9) uniform sampler2D texture_sampler;
layout(location=10) uniform bool use_texture;
layout(location=11) uniform int light_count;
layout(location=12) uniform bool isOutlinePass;

struct Light
{
    vec3 position;
    vec3 color;
    float intensity;
};

layout(std430, binding=1) buffer LightBuffer
{
    Light lights[];
};

// out /////////////////////////////////////////////////////////////////////////
out vec4 frag_color; // the resulting pixel color

void main(void)
{
    if (isOutlinePass)
    {
        frag_color = vec4(0.0, 0.5, 1.0, 1.0);
        return;
    }

    vec3 position = world_position.xyz;
    vec3 normal = world_normal.xyz;
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < light_count; ++i)
    {
        vec3 lightDir = normalize(lights[i].position - position);

        // diffuse color
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color * diffuse_coefficient;

        // specular color
        vec3 viewDir = normalize(eye_position.xyz - position);
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), specular_exponent);
        vec3 specular = spec * lights[i].color * specular_coefficient;

        // texture color
        vec3 textureColor = use_texture ? texture(texture_sampler, uv).rgb : vec3(1.0);

        // final color
        lighting += (diffuse + specular) * textureColor * lights[i].intensity;
    }
    frag_color = vec4(lighting, 1.0);
}