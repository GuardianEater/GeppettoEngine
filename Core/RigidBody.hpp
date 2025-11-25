/*****************************************************************//**
 * \file   RigidBody.hpp
 * \brief  Component for storing movement data, velocity and the like
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>

namespace Client
{
    struct PhysicsParticle
    {
        glm::vec3 position{};
        glm::vec3 previousPosition{};

        float mass = 1.0f;
    };

    struct RigidBody
    {
        glm::vec3 velocity{};
        glm::vec3 acceleration{};
        glm::vec3 rotationalVelocity{};
        glm::vec3 rotationalAcceleration{};

        std::vector<glm::vec3> restPositions;
        std::vector<glm::vec3> currentPositions;
    };
}
