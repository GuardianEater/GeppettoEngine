/*****************************************************************//**
 * \file   Transform.hpp
 * \brief  transform component, stores position data
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm.hpp>

namespace Client
{
    struct Transform
    {
        using Hidden = std::true_type;

        glm::vec3 position{};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rotation{};

        glm::vec3 previousPosition{position};
        glm::vec3 previousScale{scale};
        glm::vec3 previousRotation{rotation};
    };
}
