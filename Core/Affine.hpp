/*****************************************************************//**
 * \file   Affine.hpp
 * \brief  Useful functions for affine matrices
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <glm.hpp>

namespace Gep
{
    // inverts an affine matrix more efficiently
	glm::mat4 affine_inverse(const glm::mat4& m)
	{
		glm::mat4 result = glm::inverse(glm::mat3(m));

		const glm::mat4 transform = { {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, -glm::vec4{m[3][0], m[3][1], m[3][2], -1} };

		result *= transform;

		return result;
	}

    // crosses vec4s the same a vec 3s the w component will be set to 0
	glm::vec4 cross_product(const glm::vec4& u, const glm::vec4& v)
	{
		// uses the cross provided by glm for shortcut
		const glm::vec3 crossed = glm::cross(glm::vec3(u), glm::vec3(v));

		return glm::vec4(crossed, 0);
	}

    // generates a translation matrix to move in the direction of the given vector
    glm::mat4 translation_matrix(const glm::vec3& v)
    {
        // creates identity
        glm::mat4 translateMatrix(1);

        // sets the transform data
        translateMatrix[3][0] = v.x;
        translateMatrix[3][1] = v.y;
        translateMatrix[3][2] = v.z;
        translateMatrix[3][3] = 1.0f;

        return translateMatrix;
    }

    // generates a rotation matrix from an axis and rotation
    glm::mat4 rotation_matrix(float deg, const glm::vec4& rot_axis)
    {
        // precalculates values
        float rad = glm::radians(deg);
        float cosrad = std::cosf(rad);
        float sinrad = std::sinf(rad);
        float mag = glm::length(rot_axis);

        // gets the cos matrix
        glm::mat3 part1(cosrad);

        // getst the scaler before the outer product matrix
        float OPMScaler = (1.0f - cosrad) / (mag * mag);
        glm::mat3 OPM(0); // outer product matrix
        OPM[0][0] = rot_axis.x * rot_axis.x; OPM[1][0] = rot_axis.x * rot_axis.y; OPM[2][0] = rot_axis.x * rot_axis.z;
        OPM[0][1] = rot_axis.y * rot_axis.x; OPM[1][1] = rot_axis.y * rot_axis.y; OPM[2][1] = rot_axis.y * rot_axis.z;
        OPM[0][2] = rot_axis.z * rot_axis.x; OPM[1][2] = rot_axis.z * rot_axis.y; OPM[2][2] = rot_axis.z * rot_axis.z;

        // gets the scaler before the cross product matrix
        float CPMScaler = (sinrad / mag);
        glm::mat3 CPM(0); // cross product matrix
        CPM[0][0] = 0.0f; CPM[1][0] = -rot_axis.z; CPM[2][0] = rot_axis.y;
        CPM[0][1] = rot_axis.z;  CPM[1][1] = 0.0f; CPM[2][1] = -rot_axis.x;
        CPM[0][2] = -rot_axis.y; CPM[1][2] = rot_axis.x;  CPM[2][2] = 0.0f;

        // calculates the rotation matrix
        glm::mat4 rotationMatrix((part1 + (OPMScaler * OPM) + (CPMScaler * CPM)));

        return rotationMatrix;
    }

    // contructs an uniform scale matrix
    glm::mat4 scale_matrix(float r)
    {
        // creates a identity multiplied by r
        glm::mat4 scaleMatrix(r);

        // sets the bottom right item to 1
        scaleMatrix[3][3] = 1.0f;

        return scaleMatrix;
    }

    // constructs a non uniform scale matrix
    glm::mat4 scale_matrix(float rx, float ry, float rz)
    {
        // creats identity
        glm::mat4 scaleMatrix(1);

        // sets the scale data
        scaleMatrix[0][0] = rx;
        scaleMatrix[1][1] = ry;
        scaleMatrix[2][2] = rz;

        return scaleMatrix;
    }

    // constructs a non uniform scale matrix
    glm::mat4 scale_matrix(const glm::vec3& scale)
    {
        // creats identity
        glm::mat4 scaleMatrix(1);

        // sets the scale data
        scaleMatrix[0][0] = scale.x;
        scaleMatrix[1][1] = scale.y;
        scaleMatrix[2][2] = scale.z;

        return scaleMatrix;
    }

}
