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
    // contains material data for pbr rendering.
    struct Material
    {
        float ao;        // ambient occlusion
        float roughness;
        float metalness;
        glm::vec3 color; // this is a uniform color across an entire mesh, use an image path for more detailed

        std::filesystem::path aoTexturePath;
        std::filesystem::path roughnessTexturePath;
        std::filesystem::path metalnessTexturePath;
        std::filesystem::path diffuseTexturePath;
    };
}
