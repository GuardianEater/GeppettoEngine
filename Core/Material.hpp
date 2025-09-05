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
    // std430 compliant.
    struct alignas(16) Material
    {
        float ao;        // ambient occlusion
        float roughness;
        float metalness; float pad0;
        glm::vec3 color; float pad1;
    };
}
