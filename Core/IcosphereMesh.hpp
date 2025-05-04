/*****************************************************************//**
 * \file   IcosphereMesh.hpp
 * \brief  Sphere made of triangles
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include "Mesh.hpp"

namespace Gep
{
    struct IcosphereMesh : public Mesh
    {
        IcosphereMesh(const size_t subdivisions = 3);

    private:
        void CreateIcosahedron();
        glm::vec2 CalculateUVs(const glm::vec3& position);
        void Subdivide(size_t subdivisions);
        void RefineTriangles(size_t recursionLevel);
        size_t GetMiddlePoint(size_t p1, size_t p2);

    private:
        std::unordered_map<size_t, size_t> mMiddlePointCache;
    };
}
