/*********************************************************************
 * file:   VQS.cpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   November 13, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  implemenation for vqs
 *********************************************************************/

#include "pch.hpp"
#include "VQS.hpp"

namespace Gep
{
    VQS VQS::operator*(const VQS& local) const
    {
        VQS result{};
        result.scale = scale * local.scale;
        result.rotation = glm::normalize(rotation * local.rotation);

        glm::vec3 scaledLocalTrans = scale * local.position;
        glm::vec3 rotatedLocalTrans = rotation * scaledLocalTrans;
        result.position = position + rotatedLocalTrans;

        return result;
    }
}

