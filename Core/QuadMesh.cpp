/*****************************************************************//**
 * \file   QuadMesh.cpp
 * \brief  simple quad mesh
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "QuadMesh.hpp"

namespace Gep
{
    QuadMesh::QuadMesh()
    {
        mVertices = {
            // front
            // positions          // normals           // texture coords
            {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},           
        };

        mFaces = {
            {0, 1, 2},
            {0, 2, 3}
        };

        Normalize();
    }
}
