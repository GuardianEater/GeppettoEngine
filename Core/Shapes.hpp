/*****************************************************************//**
 * \file   Shapes.hpp
 * \brief  A collection of shapes
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   April 2025
 *********************************************************************/

#pragma once

#include <glm.hpp>

namespace Gep
{
    struct LineSegment
    {
        glm::vec3 start{};
        glm::vec3 end{};
    };

    struct Ray
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 direction{ 0.0f, 1.0f, 0.0f };

        glm::vec3 GetPoint(float t) const
        {
            return position + direction * t;
        }
    };

    struct Sphere
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        float radius = 1.0f;
    };

    // infinite plane
    struct Plane
    {
        static Plane FromTriangle(const Triangle& triangle)
        {
            const glm::vec3 normal = glm::normalize(glm::cross(triangle.points[1] - triangle.points[0], triangle.points[2] - triangle.points[0]));
            return Plane{ normal, glm::dot(normal, triangle.points[0]) };
        }

        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        float distance = 0.0f;
    };

    struct AABB
    {
        glm::vec3 min{};
        glm::vec3 max{};
    };

    struct Triangle
    {
        union
        {
            std::array<glm::vec3, 3> points;
            struct
            {
                glm::vec3 p0;
                glm::vec3 p1;
                glm::vec3 p2;
            };
        };
    };

    struct Frustum
    {
        std::array<Plane, 6> planes;
    };
}
