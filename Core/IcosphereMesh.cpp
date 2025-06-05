/*****************************************************************//**
 * \file   IcosphereMesh.cpp
 * \brief  
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "IcosphereMesh.hpp"

namespace Gep
{
    void IcosphereMesh::Subdivide(size_t subdivisions)
    {

    }

    void IcosphereMesh::RefineTriangles(size_t recursionLevel)
    {
        for (int i = 0; i < recursionLevel; i++)
        {
            std::vector<uint32_t> indices;
            for (size_t j = 0; j < indices.size(); j += 3)
            {
                size_t i0 = indices[j + 0];
                size_t i1 = indices[j + 1];
                size_t i2 = indices[j + 2];

                size_t a = GetMiddlePoint(i0, i1);
                size_t b = GetMiddlePoint(i1, i2);
                size_t c = GetMiddlePoint(i2, i0);

                indices.push_back(i0); indices.push_back(a); indices.push_back(c);
                indices.push_back(i1); indices.push_back(b); indices.push_back(a);
                indices.push_back(i2); indices.push_back(c); indices.push_back(b);
                indices.push_back(a);  indices.push_back(b); indices.push_back(c);
            }
            indices = indices;
        }
    }

    size_t IcosphereMesh::GetMiddlePoint(size_t p1, size_t p2)
    {
        // first check if we have it already
        bool firstIsSmaller = p1 < p2;
        size_t smallerIndex = firstIsSmaller ? p1 : p2;
        size_t greaterIndex = firstIsSmaller ? p2 : p1;
        size_t key = (smallerIndex << 32) + greaterIndex;

        if (mMiddlePointCache.contains(key))
        {
            return mMiddlePointCache[key];
        }
        // not in cache, calculate it
        const Vertex& point1 = vertices[p1];
        const Vertex& point2 = vertices[p2];
        Vertex middle = {
            glm::normalize((point1.position + point2.position) / 2.0f),
            glm::normalize((point1.normal + point2.normal) / 2.0f),
            CalculateUVs(glm::normalize((point1.position + point2.position)))
        };
        size_t index = vertices.size();
        vertices.push_back(middle);
        mMiddlePointCache[key] = index;
        return index;
    }


    // Create initial icosahedron
    void IcosphereMesh::CreateIcosahedron()
    {
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio

        vertices = {
            { glm::normalize(glm::vec3(-1, phi, 0)),  glm::normalize(glm::vec3(-1, phi, 0)),  {} },
            { glm::normalize(glm::vec3(1, phi, 0)),   glm::normalize(glm::vec3(1, phi, 0)),   {} },
            { glm::normalize(glm::vec3(-1, -phi, 0)), glm::normalize(glm::vec3(-1, -phi, 0)), {} },
            { glm::normalize(glm::vec3(1, -phi, 0)),  glm::normalize(glm::vec3(1, -phi, 0)),  {} },

            { glm::normalize(glm::vec3(0, -1, phi)),  glm::normalize(glm::vec3(0, -1, phi)),  {} },
            { glm::normalize(glm::vec3(0, 1, phi)),   glm::normalize(glm::vec3(0, 1, phi)),   {} },
            { glm::normalize(glm::vec3(0, -1, -phi)), glm::normalize(glm::vec3(0, -1, -phi)), {} },
            { glm::normalize(glm::vec3(0, 1, -phi)),  glm::normalize(glm::vec3(0, 1, -phi)),  {} },

            { glm::normalize(glm::vec3(phi, 0, -1)),  glm::normalize(glm::vec3(phi, 0, -1)),  {} },
            { glm::normalize(glm::vec3(phi, 0, 1)),   glm::normalize(glm::vec3(phi, 0, 1)),   {} },
            { glm::normalize(glm::vec3(-phi, 0, -1)), glm::normalize(glm::vec3(-phi, 0, -1)), {} },
            { glm::normalize(glm::vec3(-phi, 0, 1)),  glm::normalize(glm::vec3(-phi, 0, 1)),  {} },
        };

        indices = {
            0, 11, 5, 
            0, 5, 1, 
            0, 1, 7, 
            0, 7, 10, 
            0, 10, 11,

            1, 5, 9, 
            5, 11, 4, 
            11, 10, 2, 
            10, 7, 6,
            7, 1, 8, 

            3, 9, 4, 
            3, 4, 2, 
            3, 2, 6, 
            3, 6, 8, 
            3, 8, 9,

            4, 9, 5,
            2, 4, 11,
            6, 2, 10,
            8, 6, 7,
            9, 8, 1
        };
    }

    glm::vec2 IcosphereMesh::CalculateUVs(const glm::vec3& position)
    {
        float u = 0.5f + atan2(position.z, position.x) / (2 * glm::pi<float>());
        float v = 0.5f - asin(position.y) / glm::pi<float>();
        return { u, v };
    }

    IcosphereMesh::IcosphereMesh(const size_t subdivisions)
    {
        CreateIcosahedron();
        RefineTriangles(subdivisions);

        name = "Icosphere";

        Normalize();
    }
}
