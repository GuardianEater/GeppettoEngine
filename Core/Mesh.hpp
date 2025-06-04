/*****************************************************************//**
 * \file   Mesh.hpp
 * \brief  Mesh data structure used when rendering
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>
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
        std::string name = "Unnamed";

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        AABB boundingBox{};

        void CalculateBoundingBox()
        {
            boundingBox.min = glm::vec3(FLT_MAX);
            boundingBox.max = glm::vec3(-FLT_MAX);
            for (const Vertex& vertex : vertices)
            {
                boundingBox.min = glm::min(boundingBox.min, vertex.position);
                boundingBox.max = glm::max(boundingBox.max, vertex.position);
            }
        }

        // normalizes the mesh to fit within a unit cube, also calculates the bounding box
        void Normalize()
        {
            CalculateBoundingBox();

            glm::vec3 center = (boundingBox.min + boundingBox.max) * 0.5f;
            glm::vec3 size   = boundingBox.max - boundingBox.min;

            float maxDim = glm::max(size.x, glm::max(size.y, size.z));
            for (Vertex& vertex : vertices)
            {
                vertex.position = (vertex.position - center) / maxDim;
            }
        }
    };
}

