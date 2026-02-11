/*****************************************************************//**
 * \file   Affine.cpp
 * \brief  Useful functions for affine matrices
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

// pch
#include "pch.hpp"

// other
#include "VQS.hpp"

// this
#include "GLMHelp.hpp"

namespace Gep
{
    glm::mat4 AffineInverse(const glm::mat4& m)
    {
        glm::mat4 result = glm::inverse(glm::mat3(m));

        const glm::mat4 transform = { {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, -glm::vec4{m[3][0], m[3][1], m[3][2], -1} };

        result *= transform;

        return result;
    }

    glm::vec4 Cross(const glm::vec4& u, const glm::vec4& v)
    {
        const glm::vec3 crossed = glm::cross(glm::vec3(u), glm::vec3(v));
        return glm::vec4(crossed, 0.0f);
    }

    glm::vec3 EulerFromLook(const glm::vec3& look)
    {
        // orient transform so its points toward the look vector.
        // skip if the look vector is degenerate.
        if (glm::dot(look, look) > glm::epsilon<float>())
        {
            glm::vec3 forward = glm::normalize(look);

            // world-up: avoid near-parallel up/forward
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(forward, worldUp)) > 1.0f - glm::epsilon<float>())
                worldUp = glm::vec3(1.0f, 0.0f, 0.0f);

            // orthonormal basis: right, up, forward
            glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
            glm::vec3 up = glm::cross(forward, right);

            glm::mat3 rotMat(right, up, forward); // columns -> local axes aligned with world-space basis
            glm::quat orientation = glm::quat_cast(rotMat);

            return glm::degrees(glm::eulerAngles(orientation));
        }

        return glm::vec3(0.0f);
    }

    glm::quat QuatFromLook(const glm::vec3& look)
    {
        // orient transform so its points toward the look vector.
        // skip if the look vector is degenerate.
        if (glm::dot(look, look) > glm::epsilon<float>())
        {
            glm::vec3 forward = glm::normalize(look);
            glm::vec3 up = glm::vec3(0, 1, 0); // or your chosen up
            glm::vec3 right = glm::normalize(glm::cross(up, forward));
            up = glm::cross(forward, right);

            glm::mat3 rotMat(right, up, forward); // columns are basis vectors
            return glm::normalize(glm::quat_cast(rotMat));
        }

        return glm::vec3(0.0f);
    }

    glm::mat3 NormalFromModel(const glm::mat4& modelMatrix)
    {
        return glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    }

    glm::quat Derivative(const glm::quat& q, const glm::vec3& omega)
    {
        // q' = 0.5 * Omega(omega) * q where Omega is quaternion [0, wx, wy, wz]
        glm::quat wq{ 0, omega.x, omega.y, omega.z };

        return wq * q * 0.5f;
    }

    VQS Inverse(const VQS& t)
    {
        VQS inv{};

        if (t.scale.x == 0.0f ||
            t.scale.y == 0.0f ||
            t.scale.z == 0.0f)
            Gep::Log::Error("Division by zero when taking the inverse of a VQS");

        // Inverse scale (handle zero carefully)
        inv.scale = 1.0f / t.scale;

        // Inverse rotation (unit quaternion  conjugate)
        inv.rotation = glm::conjugate(t.rotation);

        // Inverse translation:
        // First undo scale and rotation on the translation
        glm::vec3 invTrans = -(inv.rotation * (inv.scale * t.position));
        inv.position = invTrans;

        return inv;
    }
}
