/*****************************************************************//**
 * \file   Conversion.h
 * \brief  converts similar types such as assimp mat4 to glm mat4
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

#include <glm/fwd.hpp>
#include <assimp/matrix4x4.h>

namespace Gep
{
    struct VQS;

    glm::mat4 ToMat4(const aiMatrix4x4& assimpMatrix);
    glm::vec3 ToVec3(const aiVector3D& v);
    glm::quat ToQuat(const aiQuaternion& q);

    VQS ToVQS(const glm::mat4& matrix);
    VQS ToVQS(const aiMatrix4x4& m);

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
}
