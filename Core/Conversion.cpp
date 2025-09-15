/*****************************************************************//**
 * \file   Conversion.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   September 2025
 *********************************************************************/

#include "pch.hpp"
#include "Conversion.h"

#include <assimp/matrix4x4.h>
#include <glm/glm.hpp>
#include "Mesh.hpp"

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

    VQS ToVQS(const glm::mat4& m)
    {
        glm::vec3 translation = glm::vec3(m[3]);
        glm::mat3 rotScale(m);

        float scale = glm::length(rotScale[0]); // assumes uniform scaling
        glm::quat rotation = glm::quat_cast(rotScale / scale);

        return { glm::vec4(translation, 1.0f), rotation, scale };
    }

    VQS ToVQS(const aiMatrix4x4& m)
    {
        return ToVQS(ToMat4(m));
    }
}

