/*****************************************************************//**
 * \file   LightComponent.hpp
 * \brief  makes an entity a light source
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Client
{
    struct Light
    {
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        float intensity{1.0f};
    };
}
