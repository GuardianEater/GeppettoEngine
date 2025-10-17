/*****************************************************************//**
 * \file   Material.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Gep
{
    struct Texture
    {
        GLuint64 handle = NULL;        // gpu side pointer
        GLuint id = num_max<GLuint>(); // bindable id
    };

    // contains material data for pbr rendering.
    struct Material
    {
        // if it has a uniform material across the entire mesh
        float ao; // ambient occlusion
        float roughness;
        float metalness;
        glm::vec3 color;

        Texture aoTexture;
        Texture roughnessTexture;
        Texture metalnessTexture;
        Texture diffuseTexture;
        Texture normalTexture;
    };
}
