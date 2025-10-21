/*********************************************************************
 * file:   CurveComponent.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   October 20, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace Client
{
    struct CurveComponent
    {
        enum class CurveType
        {
            None, // simply connects the dots linearly no computation needed
            CubicSpline,
        };

        CurveType curveType = CurveType::CubicSpline;
        std::vector<glm::vec3> controlPoints{ {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} };
        size_t subdivisions = 20;

        glm::vec3 color = {1.0f, 1.0f, 1.0f};
        bool dirty = true;
    };
}
