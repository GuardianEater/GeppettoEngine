/*****************************************************************//**
 * \file   RigidBody.hpp
 * \brief  Component for storing movement data, velocity and the like
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

// glm
#include <glm\glm.hpp>

// help
#include "uuid.hpp"

namespace Client
{
    struct Transform;
}

namespace Client
{
    struct PhysicsParticle
    {
        glm::vec3 position{};
        glm::vec3 previousPosition{};

        float mass = 1.0f;
    };

    struct Spring
    {
        gtl::uuid startEntity;
        gtl::uuid endEntity; // target entity to perform spring computation against

        float restLength = 1.0f;
        float stiffness = 50.0f;
        float damping = 1.0f;
    };

    struct RigidBody
    {
        glm::vec3 linearVelocity{};
        glm::vec3 angularVelocity{};

        glm::vec3 forceAccumulator{};
        glm::vec3 torqueAccumulator{};

        float mass    = 1.0f;
        float invMass = 1.0f;

        glm::vec3 inertia    = { 1.0f, 1.0f, 1.0f }; // diagonal
        glm::vec3 invInerita = { 1.0f, 1.0f, 1.0f }; // diagonal

        glm::vec3 LinearAcceleration() const;
        glm::vec3 AngularAcceleration(const glm::quat& worldRotation) const;
        void ApplyForce(const glm::vec3& f);
        void ApplyTorque(const glm::vec3& t);
        void ApplyForceAtPoint(const Transform& t, const glm::vec3& f, const glm::vec3& pointWorld);
        void ClearAccumulators();
    };
}
