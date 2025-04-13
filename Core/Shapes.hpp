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

    struct Frustum
    {
        std::array<Plane, 6> planes;
    };

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


    struct AABB
    {
        static AABB Combine(const AABB& l, const AABB& r)
        {
            return { glm::min(l.min, r.min) , glm::max(l.max, r.max) };
        }

        void Fatten(float amount)
        {
            min -= amount;
            max += amount;
        }

        float GetVolume() const
        {
            const glm::vec3 size = max - min;
            return size.x * size.y * size.z;
        }

        float GetSurfaceArea() const
        {
            const glm::vec3 size = max - min;
            return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
        }

        glm::vec3 min{};
        glm::vec3 max{};
    };

    struct Cube
    {
        glm::vec3 position{};
        glm::vec3 halfSize{};
        glm::vec3 rotation{};
    };
}
