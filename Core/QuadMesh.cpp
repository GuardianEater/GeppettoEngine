/*****************************************************************//**
 * \file   QuadMesh.cpp
 * \brief  simple quad mesh
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "QuadMesh.hpp"

namespace Gep
{
    QuadMesh::QuadMesh()
    {
        vertices = {
            // front
            // positions          // normals           // texture coords
            {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},           
        };

        indices = {
            0, 1, 2,
            0, 2, 3
        };

        Normalize();
    }
}
