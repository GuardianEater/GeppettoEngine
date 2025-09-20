/*****************************************************************//**
 * \file   Transform.hpp
 * \brief  transform component, stores position data
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm\glm.hpp>
#include "Affine.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm\gtx\matrix_decompose.hpp>

namespace Client
{
    struct Transform
    {
        glm::vec3 position{};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rotation{};

        glm::vec3 previousPosition{position};
        glm::vec3 previousScale{scale};
        glm::vec3 previousRotation{rotation};

        glm::mat4 GetModelMatrix() const
        {
            glm::mat4 model = Gep::translation_matrix(position)
                            * Gep::rotation(rotation)
                            * Gep::scale_matrix(scale);
            
            return model;
        }

        void SetModelMatrix(const glm::mat4& modelMatrix)
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::quat rotationQ;
            glm::decompose(modelMatrix, scale, rotationQ, position, skew, perspective);
            rotation = glm::eulerAngles(rotationQ);
        }
    };
}
