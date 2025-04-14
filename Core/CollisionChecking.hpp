/*****************************************************************//**
 * \file   CollisionChecking.hpp
 * \brief  bunch of collision checking functions
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "Shapes.hpp"
#include "glm/glm.hpp"

namespace Gep
{
    enum class PlaneIntersectionType : uint8_t
    {
        InFront,   // in the direction of the normal
        Behind,    // in the opposite direction of the normal
        Straddling, // on both sides of the plane
        Coplanar,  // on the plane
    };

    enum class FrustumIntersectionType : uint8_t
    {
        Inside,   // completely inside the frustum
        Outside,  // completely outside the frustum
        Overlaps, // partially inside the frustum, partially outside
    };

    enum class AABBIntersectionType : uint8_t
    {
        Inside,   // completely inside the AABB
        Outside,  // completely outside the AABB
        Overlaps, // partially inside the AABB, partially outside
    };

    enum class CubeIntersectionType : uint8_t
    {
        Inside,   // completely inside the cube
        Outside,  // completely outside the cube
        Overlaps, // partially inside the cube, partially outside
    };

    bool PointTriangle(const glm::vec3& point, const Triangle& triangle);
    bool RayPlane(const Ray& ray, const Plane& plane, float& t);
    bool PointAABB(const glm::vec3& point, const AABB& aabb);
    bool RayTriangle(const Ray& ray, const Triangle& triangle, float& t);
    bool RayAABB(const Ray& ray, const AABB& aabb, float& t);
    bool SphereSphere(const Sphere& sphere1, const Sphere& sphere2);
    bool RaySphere(const Ray& ray, const Sphere& sphere, float& t);
    bool PointSphere(const glm::vec3& point, const Sphere& sphere);
    glm::vec2 Barycentric(const LineSegment& line, const glm::vec3& point);
    glm::vec3 ProjectPointOntoPlane(const glm::vec3& point, const Plane& plane);
    glm::vec3 Barycentric(const Triangle& triangle, const glm::vec3& point);
    AABBIntersectionType AABBAABB(const AABB& aabb0, const AABB& aabb1);
    PlaneIntersectionType PlaneAABB(const Plane& plane, const AABB& aabb);
    PlaneIntersectionType PointPlane(const glm::vec3& point, const Plane& plane);
    PlaneIntersectionType PlaneSphere(const Plane& plane, const Sphere& sphere);
    PlaneIntersectionType PlaneTriangle(const Plane& plane, const Triangle& triangle);
    FrustumIntersectionType FrustumSphere(const Frustum& frustum, const Sphere& sphere, size_t& lastAxis);
    FrustumIntersectionType FrustumAABB(const Frustum& frustum, const AABB& aabb, size_t& lastAxis);
    FrustumIntersectionType FrustumTriangle(const Frustum& frustum, const Triangle& triangle);
    bool CubeSphere(const Cube& cube, const Sphere& sphere);
    bool CubeCube(const Cube& cube1, const Cube& cube2);
    bool RayCube(const Ray& ray, const Cube& cube, float& t);
}

