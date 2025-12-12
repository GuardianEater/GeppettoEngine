#include "pch.hpp"
#include "RigidBody.hpp"

#include "Transform.hpp"

namespace Client
{
    glm::vec3 RigidBody::LinearAcceleration() const
    {
        return forceAccumulator * invMass;
    }

    glm::vec3 RigidBody::AngularAcceleration(const glm::quat& worldRotation) const
    {
        const glm::mat3 R = glm::mat3_cast(worldRotation);
        const glm::mat3 Iinv_body = glm::diagonal3x3(invInerita);   // diag(invIx, invIy, invIz)
        const glm::mat3 Iinv_world = R * Iinv_body * glm::transpose(R);

        return Iinv_world * torqueAccumulator;
    }

    void RigidBody::ApplyForce(const glm::vec3& f)
    {
        forceAccumulator += f;
    }

    void RigidBody::ApplyTorque(const glm::vec3& t)
    {
        torqueAccumulator += t;
    }

    void RigidBody::ApplyForceAtPoint(const Transform& t, const glm::vec3& f, const glm::vec3& pointWorld)
    {
        // torque = (point - com) x f
        glm::vec3 r = pointWorld - t.world.position;
        forceAccumulator += f;
        torqueAccumulator += glm::cross(r, f);
    }

    void RigidBody::ClearAccumulators()
    {
        forceAccumulator = { 0, 0, 0 };
        torqueAccumulator = { 0, 0, 0 };
    }
}
