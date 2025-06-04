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
    struct Material
    {
        glm::vec3 color{ 1.0f, 1.0f, 1.0f };
        float ao = 1.0f;
        float roughness = 1.0f;
        float metalness = 1.0f;
    };
}
