/*********************************************************************
 * file:   VQS.hpp
 * author: travis.gronvold (travis.gronvold@digipen.edu)
 * date:   November 13, 2025
 * Copyright © 2023 DigiPen (USA) Corporation. 
 * 
 * brief:  implementation for a matrix alternative.
 *********************************************************************/

#pragma once

namespace Gep
{
    // based on International Journal of Algebra, Vol. 2, 2008, no. 19,905 - 918
    // VQS-transformation basically scales r by s, rotates the outcome by q, and then
    // translates the latter by v.
    struct VQS // VQM
    {
        glm::vec3 position{ 0,0,0 }; // translation
        glm::quat rotation{ 1,0,0,0 }; // rotation
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f }; // uniform scaling factor

        VQS operator*(const VQS& local) const;
        auto operator<=>(const VQS&) const = default;

        friend VQS operator*(const Gep::VQS& v, const glm::quat& q);
        friend glm::vec3 operator*(const VQS& vqs, const glm::vec3& p);
    };

    inline Gep::VQS operator*(const Gep::VQS& v, const glm::quat& q)
    {
        return {
            .position = v.position,
            .rotation = v.rotation * q,
            .scale = v.scale
        };
    }

    inline glm::vec3 operator*(const VQS& vqs, const glm::vec3& p)
    {
        return vqs.position + (vqs.rotation * (vqs.scale * p));
    }
}
