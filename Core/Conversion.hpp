/*****************************************************************//**
 * \file   Conversion.h
 * \brief  converts similar types such as assimp mat4 to glm mat4
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#pragma once

// fwd
namespace Gep
{
    struct VQS;
}

namespace Gep
{
    // converts assimp mat4 to glm mat4
    glm::mat4 ToMat4(const aiMatrix4x4& assimpMatrix);

    // converts VQS to glm mat4
    glm::mat4 ToMat4(const Gep::VQS& vqs);

    // converts assimp vec3 to glm vec3
    glm::vec3 ToVec3(const aiVector3D& v);

    // converts assimp quat to glm quat
    glm::quat ToQuat(const aiQuaternion& q);

    // converts glm mat4 to VQS, assumes the matrix is a combination of translation rotation and scale with no projection or skew
    VQS ToVQS(const glm::mat4& matrix);

    // converts assimp mat4 to VQS, assumes the matrix is a combination of translation rotation and scale with no projection or skew
    VQS ToVQS(const aiMatrix4x4& m);
}
