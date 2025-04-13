/*****************************************************************//**
 * \file   CollisionChecking.cpp
 * \brief  implementation for a bunch of collision checking functions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   April 2025
 *********************************************************************/

#include "pch.hpp"

#include "CollisionChecking.hpp"
#include "Affine.hpp"
#include "glm/glm.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Gep
{
    glm::vec3 ProjectPointOntoPlane(const glm::vec3& point, const Plane& plane)
    {
        if (plane.normal == glm::vec3(0.0f)) return point;

        const float distance = glm::dot(glm::normalize(plane.normal), point) - plane.distance;
        return point - plane.normal * distance;
    }

    glm::vec2 Barycentric(const LineSegment& line, const glm::vec3& point)
    {
        glm::vec3 vec = line.start - line.end;

        const float denom = glm::dot(vec, vec);
        if (denom == 0.0f) return { 0.0f, 0.0f };

        const float u = glm::dot(point - line.start, vec) / denom;
        const float v = 1.0f - u;

        return { u, v };
    }

    glm::vec3 Barycentric(const Triangle& triangle, const glm::vec3& point)
    {
        const glm::vec3 v0 = point - triangle.points[0];
        const glm::vec3 v1 = triangle.points[1] - triangle.points[0];
        const glm::vec3 v2 = triangle.points[2] - triangle.points[0];

        const float e = glm::dot(v0, v1);
        const float a = glm::dot(v1, v1);
        const float b = glm::dot(v2, v1);

        const float f = glm::dot(v0, v2);
        const float c = glm::dot(v1, v2);
        const float d = glm::dot(v2, v2);

        const float denom = a * d - b * c;

        if (denom == 0.0f) return { 0.0f, 0.0f, 0.0f };

        const float u = (d * e - b * f) / denom;
        const float v = (a * f - c * e) / denom;
        const float w = 1.0f - u - v;

        return { u, v, w };
    }

    bool PointTriangle(const glm::vec3& point, const Triangle& triangle)
    {
        const glm::vec3 barycentric = Barycentric(triangle, point);
        return barycentric.x >= 0.0f && barycentric.y >= 0.0f && barycentric.z >= 0.0f;
    }

    bool RayPlane(const Ray& ray, const Plane& plane, float& t)
    {
        const float dotProduct = glm::dot(plane.normal, ray.direction);

        if (std::abs(dotProduct) < 1e-6f)
        {
            return false;
        }

        const float distanceToPlane = glm::dot(plane.normal, ray.position) - plane.distance;

        t = -distanceToPlane / dotProduct;

        return t >= 0;
    }

    bool PointAABB(const glm::vec3& point, const AABB& aabb)
    {
        return point.x >= aabb.min.x && point.x <= aabb.max.x &&
            point.y >= aabb.min.y && point.y <= aabb.max.y &&
            point.z >= aabb.min.z && point.z <= aabb.max.z;
    }

    bool RayTriangle(const Ray& ray, const Triangle& triangle, float& t)
    {
        const Plane plane = Plane::FromTriangle(triangle);

        if (RayPlane(ray, plane, t))
        {
            const glm::vec3 point = ray.position + ray.direction * t;

            if (PointTriangle(point, triangle))
            {
                return true;
            }
        }

        return false;
    }

    bool RayAABB(const Ray& ray, const AABB& aabb, float& t)
    {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; ++i)
        {
            const float invDir = 1.0f / ray.direction[i];
            float t1 = (aabb.min[i] - ray.position[i]) * invDir;
            float t2 = (aabb.max[i] - ray.position[i]) * invDir;

            if (t1 > t2) std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax) return false;
        }

        t = tMin;
        return true;
    }

    bool SphereSphere(const Sphere& sphere1, const Sphere& sphere2)
    {
        return glm::distance(sphere1.position, sphere2.position) <= sphere1.radius + sphere2.radius;
    }

    AABBIntersectionType AABBAABB(const AABB& aabb0, const AABB& aabb1)
    {
        bool x = aabb0.max.x < aabb1.min.x || aabb0.min.x > aabb1.max.x;
        bool y = aabb0.max.y < aabb1.min.y || aabb0.min.y > aabb1.max.y;
        bool z = aabb0.max.z < aabb1.min.z || aabb0.min.z > aabb1.max.z;

        if (x || y || z)
            return AABBIntersectionType::Outside;

        bool xFull = aabb0.min.x >= aabb1.min.x && aabb0.max.x <= aabb1.max.x;
        bool yFull = aabb0.min.y >= aabb1.min.y && aabb0.max.y <= aabb1.max.y;
        bool zFull = aabb0.min.z >= aabb1.min.z && aabb0.max.z <= aabb1.max.z;

        if (xFull && yFull && zFull)
            return AABBIntersectionType::Inside;

        return AABBIntersectionType::Overlaps;
    }

    bool RaySphere(const Ray& ray, const Sphere& sphere, float& t)
    {
        const glm::vec3 L = ray.position - sphere.position;

        const float a = glm::dot(ray.direction, ray.direction);
        const float b = 2 * glm::dot(ray.direction, L);
        const float c = glm::dot(L, L) - sphere.radius * sphere.radius;

        const float discriminant = b * b - 4 * a * c;
        const float sqrtDiscriminant = sqrtf(discriminant);

        if (discriminant < 0)
        {
            t = 0.0f;
            return false;
        }

        const float t1 = (-b - sqrtDiscriminant) / (2 * a);
        const float t2 = (-b + sqrtDiscriminant) / (2 * a);

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
            return false;
        }

        return true;
    }

    bool PointSphere(const glm::vec3& point, const Sphere& sphere)
    {
        return glm::distance(point, sphere.position) <= sphere.radius;
    }

    PlaneIntersectionType PlaneAABB(const Plane& plane, const AABB& aabb)
    {
        glm::vec3 positiveVertex = aabb.min;
        glm::vec3 negativeVertex = aabb.max;

        if (plane.normal.x >= 0)
        {
            positiveVertex.x = aabb.max.x;
            negativeVertex.x = aabb.min.x;
        }
        if (plane.normal.y >= 0)
        {
            positiveVertex.y = aabb.max.y;
            negativeVertex.y = aabb.min.y;
        }
        if (plane.normal.z >= 0)
        {
            positiveVertex.z = aabb.max.z;
            negativeVertex.z = aabb.min.z;
        }

        const float dPositive = glm::dot(plane.normal, positiveVertex) - plane.distance;
        const float dNegative = glm::dot(plane.normal, negativeVertex) - plane.distance;

        if (dPositive < 0)
        {
            return PlaneIntersectionType::Behind;
        }
        else if (dNegative > 0)
        {
            return PlaneIntersectionType::InFront;
        }
        else
        {
            return PlaneIntersectionType::Stradding;
        }
    }

    PlaneIntersectionType PointPlane(const glm::vec3& point, const Plane& plane)
    {
        const float distance = glm::dot(glm::normalize(plane.normal), point) - plane.distance;

        if (distance > 0.0f)
        {
            return PlaneIntersectionType::InFront;
        }
        else if (distance < 0.0f)
        {
            return PlaneIntersectionType::Behind;
        }
        else
        {
            return PlaneIntersectionType::Coplanar;
        }
    }

    PlaneIntersectionType PlaneSphere(const Plane& plane, const Sphere& sphere)
    {
        const float distance = glm::dot(plane.normal, sphere.position) - plane.distance;

        if (distance > sphere.radius)
        {
            return PlaneIntersectionType::InFront;
        }
        else if (distance < -sphere.radius)
        {
            return PlaneIntersectionType::Behind;
        }
        else
        {
            return PlaneIntersectionType::Stradding;
        }
    }

    PlaneIntersectionType PlaneTriangle(const Plane& plane, const Triangle& triangle)
    {
        PlaneIntersectionType p0_intersection = PointPlane(triangle.p0, plane);
        PlaneIntersectionType p1_intersection = PointPlane(triangle.p1, plane);
        PlaneIntersectionType p2_intersection = PointPlane(triangle.p2, plane);

        const bool behind = p0_intersection == PlaneIntersectionType::Behind
            || p1_intersection == PlaneIntersectionType::Behind
            || p2_intersection == PlaneIntersectionType::Behind;

        const bool inFront = p0_intersection == PlaneIntersectionType::InFront
            || p1_intersection == PlaneIntersectionType::InFront
            || p2_intersection == PlaneIntersectionType::InFront;

        if (behind && inFront)
        {
            return PlaneIntersectionType::Stradding;
        }
        else if (behind)
        {
            return PlaneIntersectionType::Behind;
        }
        else if (inFront)
        {
            return PlaneIntersectionType::InFront;
        }
        else
        {
            return PlaneIntersectionType::Coplanar;
        }
    }

    FrustumIntersectionType FrustumSphere(const Frustum& frustum, const Sphere& sphere, size_t& lastAxis)
    {
        FrustumIntersectionType result = FrustumIntersectionType::Inside;
        PlaneIntersectionType lastAxisResult = PlaneSphere(frustum.planes[lastAxis], sphere);

        if (lastAxisResult == PlaneIntersectionType::Behind)
        {
            return FrustumIntersectionType::Outside;
        }
        else if (lastAxisResult == PlaneIntersectionType::Stradding)
        {
            result = FrustumIntersectionType::Overlaps;
        }

        size_t index = 0;
        for (const Plane& plane : frustum.planes)
        {
            if (index == lastAxis) continue;

            PlaneIntersectionType planeResult = PlaneSphere(plane, sphere);

            if (planeResult == PlaneIntersectionType::Behind)
            {
                lastAxis = index;
                return FrustumIntersectionType::Outside;
            }
            else if (planeResult == PlaneIntersectionType::Stradding)
            {
                result = FrustumIntersectionType::Overlaps;
                lastAxis = index;
            }

            ++index;
        }

        return result;
    }

    FrustumIntersectionType FrustumAABB(const Frustum& frustum, const AABB& aabb, size_t& lastAxis)
    {
        FrustumIntersectionType result = FrustumIntersectionType::Inside;
        PlaneIntersectionType lastAxisResult = PlaneAABB(frustum.planes[lastAxis], aabb);

        if (lastAxisResult == PlaneIntersectionType::Behind)
        {
            return FrustumIntersectionType::Outside;
        }
        else if (lastAxisResult == PlaneIntersectionType::Stradding)
        {
            result = FrustumIntersectionType::Overlaps;
        }

        size_t index = 0;
        for (const Plane& plane : frustum.planes)
        {
            if (index == lastAxis) continue;

            PlaneIntersectionType planeResult = PlaneAABB(plane, aabb);

            if (planeResult == PlaneIntersectionType::Behind)
            {
                lastAxis = index;
                return FrustumIntersectionType::Outside;
            }
            else if (planeResult == PlaneIntersectionType::Stradding)
            {
                result = FrustumIntersectionType::Overlaps;
                lastAxis = index;
            }

            ++index;
        }

        return result;
    }

    FrustumIntersectionType FrustumTriangle(const Frustum& frustum, const Triangle& triangle)
    {
        FrustumIntersectionType result = FrustumIntersectionType::Inside;

        for (const Plane& plane : frustum.planes)
        {
            PlaneIntersectionType planeResult = PlaneTriangle(plane, triangle);
            if (planeResult == PlaneIntersectionType::Behind)
            {
                return FrustumIntersectionType::Outside;
            }
            else if (planeResult == PlaneIntersectionType::Stradding)
            {
                result = FrustumIntersectionType::Overlaps;
            }
        }

        return result;
    }

    bool CubeSphere(const Cube& cube, const Sphere& sphere)
    {
        glm::mat3 rotationMatrix = glm::mat3(Gep::rotation(cube.rotation));

        glm::vec3 localSpherePos = glm::transpose(rotationMatrix) * (sphere.position - cube.position);

        glm::vec3 min = -cube.halfSize;
        glm::vec3 max = cube.halfSize;

        // closest point on AABB to sphere center
        glm::vec3 closest = glm::clamp(localSpherePos, min, max);

        // distance from closest point to sphere center
        glm::vec3 delta = localSpherePos - closest;
        float distSq = glm::dot(delta, delta);

        return distSq <= (sphere.radius * sphere.radius);
    }

    // projects the OBB onto the given axis and returns the min and max values of the projection
    static void ProjectOBB(const glm::vec3& center, const glm::vec3 halfSize, const glm::mat3& orientation, const glm::vec3& axis, float& outMin, float& outMax) 
    {
        float projection = glm::dot(center, axis);
        float r = 0.0f;
        r += std::abs(halfSize.x * glm::dot(orientation[0], axis));
        r += std::abs(halfSize.y * glm::dot(orientation[1], axis));
        r += std::abs(halfSize.z * glm::dot(orientation[2], axis));

        outMin = projection - r;
        outMax = projection + r;
    }

    static bool OverlapOnAxis(const Cube& cube1, const Cube& cube2, const glm::vec3& axis) 
    {
        if (glm::dot(axis, axis) < 1e-6f) return true;

        glm::vec3 axisNormalized = glm::normalize(axis);

        glm::mat3 orientA = Gep::rotation(cube1.rotation);
        glm::mat3 orientB = Gep::rotation(cube2.rotation);

        float minA, maxA;
        float minB, maxB;

        ProjectOBB(cube1.position, cube1.halfSize, orientA, axisNormalized, minA, maxA);
        ProjectOBB(cube2.position, cube2.halfSize, orientB, axisNormalized, minB, maxB);

        return !(maxA < minB || maxB < minA); // overlap exists
    }

    bool CubeCube(const Cube& cube1, const Cube& cube2)
    {
        glm::mat3 axisA = Gep::rotation(cube1.rotation);
        glm::mat3 axisB = Gep::rotation(cube2.rotation);

        glm::vec3 axesToTest[15] = {
            axisA[0], axisA[1], axisA[2],
            axisB[0], axisB[1], axisB[2],
            glm::cross(axisA[0], axisB[0]), glm::cross(axisA[0], axisB[1]), glm::cross(axisA[0], axisB[2]),
            glm::cross(axisA[1], axisB[0]), glm::cross(axisA[1], axisB[1]), glm::cross(axisA[1], axisB[2]),
            glm::cross(axisA[2], axisB[0]), glm::cross(axisA[2], axisB[1]), glm::cross(axisA[2], axisB[2]),
        };

        for (int i = 0; i < 15; ++i)
        {
            if (!OverlapOnAxis(cube1, cube2, axesToTest[i])) {
                return false; // separating axis found
            }
        }

        return true; // no separating axis -> collision
    }
    
}

