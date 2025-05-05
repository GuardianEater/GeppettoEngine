#version 430

// in variables ////////////////////////////////////////////////////////////////
layout(location=0) /*flat*/ in vec4 world_normal;   // the normal vector of the surface hit
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
layout(location=12) uniform bool isSolidColor;
layout(location=13) uniform vec3 solidColor;
layout(location=14) uniform bool isHighlighted;

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
    // Normalize surface normal and compute view vector.
    vec3 N = normalize(world_normal.xyz);
    vec3 V = normalize(eye_position.xyz - world_position.xyz);
    
    // Determine the base color from texture or solid color.
    vec3 baseColor = vec3(1.0);
    if (isSolidColor) {
        frag_color = vec4(solidColor, 1);
        return;
    } else if (use_texture) {
        baseColor = texture(texture_sampler, uv).rgb;
    }
    
    // Start with ambient contribution.
    vec3 color = ambient_color * baseColor;
    
    // === Hardcoded Sky Light ===
//    vec3 skyLightDir = normalize(vec3(0.0, 1.0, 0.0)); // Light from above
//    vec3 skyLightColor = vec3(0.6, 0.7, 1.0); // Slightly bluish tint
//    float skyLightIntensity = 1.0; // Adjust as needed
//    // Diffuse from sky light
//    float NdotL_sky = max(dot(N, skyLightDir), 0.0);
//    vec3 skyDiffuse = skyLightColor * NdotL_sky * skyLightIntensity;
//    // Specular from sky light
//    vec3 H_sky = normalize(V + skyLightDir); // Blinn-Phong half-vector
//    float NdotH_sky = max(dot(N, H_sky), 0.0);
//    vec3 skySpecular = specular_coefficient * skyLightColor * pow(NdotH_sky, specular_exponent);
//    // Add sky light contribution
//    color += (baseColor * skyDiffuse + skySpecular);
//    // ============================

    // Loop over each light.
    for (int i = 0; i < light_count; i++) {
        // Compute the light vector.
        vec3 lightVector = lights[i].position - world_position.xyz;
        float lightDistance = length(lightVector);
        vec3 L = normalize(lightVector);

        float attenuation = clamp(1.0 - (lightDistance / lights[i].intensity), 0.0, 1.0);
        
        // Diffuse term (Lambertian)
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = diffuse_coefficient * lights[i].color * NdotL;
        
        // Specular term (Blinn Phong)
        vec3 H = normalize(V + L); // half-vector between view and light
        float NdotH = max(dot(N, H), 0.0);
        vec3 specular = specular_coefficient * lights[i].color * pow(NdotH, specular_exponent);
        
        // Accumulate contributions.
        color += (baseColor * diffuse + specular) * attenuation;
    }
    
    // Optionally apply a highlight effect.
    if (isHighlighted) {
        color = mix(color, vec3(1.0, 1.0, 0.0), 0.9);
    }
    
    frag_color = vec4(color, 1.0);
}