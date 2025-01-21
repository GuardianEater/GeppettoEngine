/*****************************************************************//**
 * \file   Icosphere.hpp
 * \brief  Sphere made of triangles
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include "Mesh.hpp"

namespace Gep
{
    struct Icosphere : public Mesh
    {
        Icosphere(const size_t subdivisions = 3);

    private:
        void CreateIcosahedron();
        void Subdivide(size_t subdivisions);
        void RefineTriangles(size_t recursionLevel);
        size_t GetMiddlePoint(size_t p1, size_t p2);

    private:
        std::unordered_map<size_t, size_t> mMiddlePointCache;
    };
}
