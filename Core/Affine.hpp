/*****************************************************************//**
 * \file   Affine.hpp
 * \brief  Useful functions for affine matrices
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glm\glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace Gep
{
    glm::mat4 affine_inverse(const glm::mat4& m);
    glm::vec4 cross_product(const glm::vec4& u, const glm::vec4& v);
    glm::mat4 translation_matrix(const glm::vec3& v);
    glm::mat4 rotation_matrix(float deg, const glm::vec4& rot_axis);
    glm::mat4 scale_matrix(float r);
    glm::mat4 scale_matrix(float rx, float ry, float rz);
    glm::mat4 scale_matrix(const glm::vec3& scale);
    glm::mat4 perspective(const glm::vec3& viewport, float near, float far);
    glm::mat4 rotation(const glm::vec3& eulerRotation);
    void yaw(float angle, glm::vec3& right, glm::vec3& back, const glm::vec3& up);
    void pitch(float angle, const glm::vec3& right, glm::vec3& back, glm::vec3& up);
    void roll(float angle, glm::vec3& right, const glm::vec3& back, glm::vec3& up);
}
