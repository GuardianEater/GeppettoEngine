/*****************************************************************//**
 * \file   Affine.hpp
 * \brief  Useful functions for affine matrices
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

namespace Gep
{
    struct VQS;
}

namespace Gep
{
    // checks if a type is supported by glm::inverse
    template <typename T>
    concept TypeIsGLMInversable = requires(const T & t)
    {
        { glm::inverse(t) };
    };
}

namespace Gep
{
    // more efficient inverse for affine matrices, assumes no projection and that the bottom row is [0, 0, 0, 1]
    glm::mat4 AffineInverse(const glm::mat4& m);

    // cross product for vec4's, assumes w is 0 and returns a vec4 with w of 0
    glm::vec4 Cross(const glm::vec4& u, const glm::vec4& v);

    /// given a look vector returns the euler angles, assumes +y up.
    glm::vec3 EulerFromLook(const glm::vec3& look);

    /// given a look vector returns a quat of that look vector.
    glm::quat QuatFromLook(const glm::vec3& look);

    /// creates a normal matrix from a passed model matrix
    glm::mat3 NormalFromModel(const glm::mat4& modelMatrix);

    // q' = 0.5 * Omega(omega) * q where Omega is quaternion [0, wx, wy, wz]
    glm::quat Derivative(const glm::quat& q, const glm::vec3& omega);

    // inverse for any type that glm::inverse supports
    template <typename T>
        requires TypeIsGLMInversable<T>
    T Inverse(const T& t)
    {
        return glm::inverse(t);
    }

    // specialized inverse for VQS
    Gep::VQS Inverse(const VQS& t);

    template <typename T>
        requires std::is_floating_point_v<T>
    T AngleBetween(const glm::vec<3, T>& a, const glm::vec<3, T> b)
    {
        return glm::acos(glm::dot(a, b) / (glm::length(a) * glm::length(b)));
    }
}
