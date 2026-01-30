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
        GLuint64 handle = NULL; // gpu side pointer
        GLuint id       = NULL; // bindable id
    };

    // contains material data for pbr rendering.
    struct Material
    {
        // if it has a uniform material across the entire mesh
        float ao = 0.8f; // ambient occlusion
        float roughness = 0.8f;
        float metalness = 0.8f;
        glm::vec4 color = {0.7f, 0.7f, 0.7f, 1.0f};

        Texture aoTexture{};
        Texture roughnessTexture{};
        Texture metalnessTexture{};
        Texture diffuseTexture{};
        Texture normalTexture{};
    };
}
