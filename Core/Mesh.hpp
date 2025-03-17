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

        void Normalize()
        {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
            for (const Vertex& vertex : mVertices)
            {
                min = glm::min(min, vertex.position);
                max = glm::max(max, vertex.position);
            }
            glm::vec3 center = (min + max) * 0.5f;
            glm::vec3 size = max - min;
            float maxDim = glm::max(size.x, glm::max(size.y, size.z));
            for (Vertex& vertex : mVertices)
            {
                vertex.position = (vertex.position - center) / maxDim;
            }
        }

        void MergeVertices(float epsilon = 0.0001f)
        {
            std::vector<Vertex> uniqueVertices;
            std::vector<GLuint> vertexMap(mVertices.size(), 0);
            for (GLuint i = 0; i < mVertices.size(); ++i)
            {
                bool found = false;
                for (GLuint j = 0; j < uniqueVertices.size(); ++j)
                {
                    if (glm::distance(mVertices[i].position, uniqueVertices[j].position) < epsilon)
                    {
                        vertexMap[i] = j;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    vertexMap[i] = static_cast<GLuint>(uniqueVertices.size());
                    uniqueVertices.push_back(mVertices[i]);
                }
            }
            for (Face& face : mFaces)
            {
                for (GLuint i = 0; i < 3; ++i)
                {
                    face[i] = vertexMap[i];
                }
            }
            mVertices = uniqueVertices;
        }
    };

    struct MaterialData
    {
        glm::vec3 diffuse = { 2.0f, 2.0f, 2.0f }; // color
        glm::vec3 specular = { 0.5f, 0.5f, 0.5f }; // shine color
        float specularExponent = 5; // amount of shine;
        GLuint texture{};
    };

    struct Model
    {
        std::vector<Mesh> mMeshes{};
        std::vector<GLuint> mTextures{};
    };
}

