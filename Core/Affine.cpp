/*****************************************************************//**
 * \file   Affine.cpp
 * \brief  Useful functions for affine matrices
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "Affine.hpp"

namespace Gep
{
    glm::mat4 affine_inverse(const glm::mat4& m)
    {
        glm::mat4 result = glm::inverse(glm::mat3(m));

        const glm::mat4 transform = { {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, -glm::vec4{m[3][0], m[3][1], m[3][2], -1} };

        result *= transform;

        return result;
    }

    glm::vec4 cross_product(const glm::vec4& u, const glm::vec4& v)
    {
        const glm::vec3 crossed = glm::cross(glm::vec3(u), glm::vec3(v));
        return glm::vec4(crossed, 0);
    }

    glm::mat4 translation_matrix(const glm::vec3& v)
    {
        glm::mat4 translateMatrix(1);
        translateMatrix[3][0] = v.x;
        translateMatrix[3][1] = v.y;
        translateMatrix[3][2] = v.z;
        translateMatrix[3][3] = 1.0f;
        return translateMatrix;
    }

    glm::mat4 rotation_matrix(float deg, const glm::vec4& rot_axis)
    {
        float rad = glm::radians(deg);
        float cosrad = std::cosf(rad);
        float sinrad = std::sinf(rad);
        float mag = glm::length(rot_axis);

        glm::mat3 part1(cosrad);

        float OPMScaler = (1.0f - cosrad) / (mag * mag);
        glm::mat3 OPM(0);
        OPM[0][0] = rot_axis.x * rot_axis.x; OPM[1][0] = rot_axis.x * rot_axis.y; OPM[2][0] = rot_axis.x * rot_axis.z;
        OPM[0][1] = rot_axis.y * rot_axis.x; OPM[1][1] = rot_axis.y * rot_axis.y; OPM[2][1] = rot_axis.y * rot_axis.z;
        OPM[0][2] = rot_axis.z * rot_axis.x; OPM[1][2] = rot_axis.z * rot_axis.y; OPM[2][2] = rot_axis.z * rot_axis.z;

        float CPMScaler = (sinrad / mag);
        glm::mat3 CPM(0);
        CPM[0][0] = 0.0f; CPM[1][0] = -rot_axis.z; CPM[2][0] = rot_axis.y;
        CPM[0][1] = rot_axis.z;  CPM[1][1] = 0.0f; CPM[2][1] = -rot_axis.x;
        CPM[0][2] = -rot_axis.y; CPM[1][2] = rot_axis.x;  CPM[2][2] = 0.0f;

        glm::mat4 rotationMatrix((part1 + (OPMScaler * OPM) + (CPMScaler * CPM)));
        return rotationMatrix;
    }

    glm::mat4 scale_matrix(float r)
    {
        glm::mat4 scaleMatrix(r);
        scaleMatrix[3][3] = 1.0f;
        return scaleMatrix;
    }

    glm::mat4 scale_matrix(float rx, float ry, float rz)
    {
        glm::mat4 scaleMatrix(1);
        scaleMatrix[0][0] = rx;
        scaleMatrix[1][1] = ry;
        scaleMatrix[2][2] = rz;
        return scaleMatrix;
    }

    glm::mat4 scale_matrix(const glm::vec3& scale)
    {
        glm::mat4 scaleMatrix(1);
        scaleMatrix[0][0] = scale.x;
        scaleMatrix[1][1] = scale.y;
        scaleMatrix[2][2] = scale.z;
        return scaleMatrix;
    }

    glm::mat4 perspective(const glm::vec3& viewport, float near, float far)
    {
        glm::mat4 result(0);
        result[0][0] = (2.0f * viewport.z) / viewport.x;
        result[1][1] = (2.0f * viewport.z) / viewport.y;
        result[2][2] = (near + far) / (near - far);
        result[3][2] = (2.0f * near * far) / (near - far);
        result[2][3] = -1;
        return result;
    }

    glm::mat4 rotation(const glm::vec3& eulerRotation)
    {
        const glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(eulerRotation.x), glm::vec3(1, 0, 0));
        const glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(eulerRotation.y), glm::vec3(0, 1, 0));
        const glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(eulerRotation.z), glm::vec3(0, 0, 1));
        return rotationZ * rotationY * rotationX;
    }

    void yaw(float angle, glm::vec3& right, glm::vec3& back, const glm::vec3& up)
    {
        const glm::mat4 rotate = rotation_matrix(angle, { up, 0 });
        right = rotate * glm::vec4(right, 0);
        back = rotate * glm::vec4(back, 0);
    }

    void pitch(float angle, const glm::vec3& right, glm::vec3& back, glm::vec3& up)
    {
        const glm::mat4 rotate = rotation_matrix(angle, { right, 0 });
        up = rotate * glm::vec4(up, 0);
        back = rotate * glm::vec4(back, 0);
    }

    void roll(float angle, glm::vec3& right, const glm::vec3& back, glm::vec3& up)
    {
        const glm::mat4 rotate = rotation_matrix(angle, { back, 0 });
        right = rotate * glm::vec4(right, 0);
        up = rotate * glm::vec4(up, 0);
    }
}
