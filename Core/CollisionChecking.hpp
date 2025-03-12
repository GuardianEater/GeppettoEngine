/*****************************************************************//**
 * \file   CollisionChecking.hpp
 * \brief  bunch of collision checking functions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "glm/glm.hpp"

namespace Gep
{
    enum class PlaneIntersectionType
    {
        InFront,
        Behind,
        Overlaps,
    };

    enum class FrustumIntersectionType
    {
        Inside,
        Outside,
        Overlaps,
    };

    bool RaySphere(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius, float& t)
    {
        const glm::vec3 L = rayStart - sphereCenter;

        const float a = glm::dot(rayDir, rayDir);
        const float b = 2 * glm::dot(rayDir, L);
        const float c = glm::dot(L, L) - sphereRadius * sphereRadius;

        const float discriminant = b * b - 4 * a * c;
        const float sqrtDiscriminant = sqrtf(discriminant);

        if (discriminant < 0)
        {
            t = 0.0f;
            return false;
        }

        // compute the closest intersection point (smallest positive t)
        const float t1 = (-b - sqrtDiscriminant) / (2 * a);
        const float t2 = (-b + sqrtDiscriminant) / (2 * a);

        // choose the smallest positive t
        if (t1 >= 0)
        {
            t = t1;
        }
        else if (t2 >= 0)
        {
            t = 0.0f;
        }
        else
        {
            t = 0.0f;
            return false; // both t1 and t2 are negative, ray starts after the sphere
        }

        return true;
    }

    PlaneIntersectionType PlaneSphere(const glm::vec4& plane, const glm::vec3& sphereCenter, float sphereRadius)
    {
        const float distance = glm::dot(glm::vec3(plane), sphereCenter) - plane.w;

        if (distance > sphereRadius)
        {
            return PlaneIntersectionType::InFront;
        }
        else if (distance < -sphereRadius)
        {
            return PlaneIntersectionType::Behind;
        }
        else
        {
            return PlaneIntersectionType::Overlaps;
        }
    }

    FrustumIntersectionType FrustumSphere(const glm::vec4 planes[6], const glm::vec3& sphereCenter, float sphereRadius, size_t& lastAxis)
    {
        FrustumIntersectionType result = FrustumIntersectionType::Inside;
        for (size_t i = 0; i < 6; ++i)
        {
            // if the sphere is outside on any plane, return Outside
            // if the sphere is inside on all planes, return Inside
            // if the sphere is overlapping on any plane, and none are outside, return Overlaps

            PlaneIntersectionType planeResult = PlaneSphere(planes[i], sphereCenter, sphereRadius);

            if (planeResult == PlaneIntersectionType::Behind)
            {
                lastAxis = i;
                return FrustumIntersectionType::Outside;
            }
            else if (planeResult == PlaneIntersectionType::Overlaps)
            {
                result = FrustumIntersectionType::Overlaps;
                lastAxis = i;
            }
        }

        return result;
    }

    // the t is where the ray intersects the plane, along the ray
    bool RayPlane(const glm::vec3& rayStart, const glm::vec3& rayDir, const glm::vec4& plane, float& t)
    {
        const glm::vec3 planeNormal(plane.x, plane.y, plane.z);
        const float planeDistance = plane.w;

        const float dotProduct = glm::dot(planeNormal, rayDir);

        // if the ray is parallel to the plane
        if (std::abs(dotProduct) < 1e-6f)
        {
            return false;
        }

        const float distanceToPlane = glm::dot(planeNormal, rayStart) - planeDistance;

        t = -distanceToPlane / dotProduct;

        // if the intersection is in the positive direction of the ray
        return t >= 0;
    }
}
