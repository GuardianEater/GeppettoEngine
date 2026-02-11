/*****************************************************************//**
 * \file   Conversion.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#include "pch.hpp"
#include "Conversion.hpp"

#include "VQS.hpp"

namespace Gep
{
    glm::mat4 ToMat4(const aiMatrix4x4& m)
    {
        glm::mat4 out{};
        out[0][0] = m.a1; out[1][0] = m.a2; out[2][0] = m.a3; out[3][0] = m.a4;
        out[0][1] = m.b1; out[1][1] = m.b2; out[2][1] = m.b3; out[3][1] = m.b4;
        out[0][2] = m.c1; out[1][2] = m.c2; out[2][2] = m.c3; out[3][2] = m.c4;
        out[0][3] = m.d1; out[1][3] = m.d2; out[2][3] = m.d3; out[3][3] = m.d4;
        return out;
    }

    glm::mat4 ToMat4(const Gep::VQS& vqs)
    {
        return glm::translate(glm::mat4(1.0f), vqs.position) *
               glm::mat4_cast(vqs.rotation) *
               glm::scale(glm::mat4(1.0f), vqs.scale);
    }

    glm::vec3 ToVec3(const aiVector3D& v)
    {
        return { v.x, v.y, v.z };
    }

    glm::quat ToQuat(const aiQuaternion& q)
    {
        return glm::quat(q.w, q.x, q.y, q.z);
    }

    VQS ToVQS(const glm::mat4& m)
    {
        glm::vec3 translation = glm::vec3(m[3]);
        glm::mat3 rotScale(m);
        glm::vec3 scale
        {
            glm::length(rotScale[0]), // length of X basis vector
            glm::length(rotScale[1]), // length of Y basis vector
            glm::length(rotScale[2])  // length of Z basis vector        
        };

        glm::mat3 rotationMat{};
        rotationMat[0] = rotScale[0] / scale.x;
        rotationMat[1] = rotScale[1] / scale.y;
        rotationMat[2] = rotScale[2] / scale.z;

        glm::quat rotation = glm::quat_cast(rotationMat);
        return { glm::vec4(translation, 1.0f), rotation, scale };
    }

    VQS ToVQS(const aiMatrix4x4& m)
    {
        return ToVQS(ToMat4(m));
    }
}

