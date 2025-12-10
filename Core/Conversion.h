/*****************************************************************//**
 * \file   Conversion.h
 * \brief  converts similar types such as assimp mat4 to glm mat4
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

namespace Gep
{
    struct VQS;

    glm::mat4 ToMat4(const aiMatrix4x4& assimpMatrix);
    glm::mat4 ToMat4(const Gep::VQS& vqs);

    glm::vec3 ToVec3(const aiVector3D& v);
    glm::quat ToQuat(const aiQuaternion& q);

    VQS ToVQS(const glm::mat4& matrix);
    VQS ToVQS(const aiMatrix4x4& m);

    glm::quat Derivative(const glm::quat& q, const glm::vec3& omega);

    template <typename T>
    concept TypeIsGLMInversable = requires(const T & t)
    {
        { glm::inverse(t) };
    };

    template <typename T>
        requires TypeIsGLMInversable<T>
    T Inverse(const T& t)
    {
        return glm::inverse(t);
    }

    VQS Inverse(const VQS& t);

    template <typename T>
        requires std::is_floating_point_v<T>
    T AngleBetween(const glm::vec<3, T>& a, const glm::vec<3, T> b)
    {
        return glm::acos(glm::dot(a, b) / (glm::length(a) * glm::length(b)));
    }
}
