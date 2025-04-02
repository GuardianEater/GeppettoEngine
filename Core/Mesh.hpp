/*****************************************************************//**
 * \file   Mesh.hpp
 * \brief  Mesh data structure used when rendering
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>
#include <glm.hpp>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>

#include "Shapes.hpp"

namespace Gep
{
    /**
     * Typical Mesh.
     */
    struct Vertex
    {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 texCoord{};
    };

    struct Mesh
    {
        using Face = glm::vec<3, GLuint>;
        using Edge = glm::vec<2, GLuint>;

        std::vector<Vertex> mVertices{};
        std::vector<Face> mFaces{};

        AABB mBoundingBox{};

        void CalculateBoundingBox()
        {
            mBoundingBox.min = glm::vec3(FLT_MAX);
            mBoundingBox.max = glm::vec3(-FLT_MAX);
            for (const Vertex& vertex : mVertices)
            {
                mBoundingBox.min = glm::min(mBoundingBox.min, vertex.position);
                mBoundingBox.max = glm::max(mBoundingBox.max, vertex.position);
            }
        }

        // normalizes the mesh to fit within a unit cube, also calculates the bounding box
        void Normalize()
        {
            CalculateBoundingBox();

            glm::vec3 center = (mBoundingBox.min + mBoundingBox.max) * 0.5f;
            glm::vec3 size   = mBoundingBox.max - mBoundingBox.min;

            float maxDim = glm::max(size.x, glm::max(size.y, size.z));
            for (Vertex& vertex : mVertices)
            {
                vertex.position = (vertex.position - center) / maxDim;
            }
        }
    };
}

