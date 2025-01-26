/*****************************************************************//**
 * \file   Icosphere.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "IcosphereMesh.hpp"

namespace Gep
{
    void Icosphere::Subdivide(size_t subdivisions)
    {

    }

    void Icosphere::RefineTriangles(size_t recursionLevel)
    {
        for (int i = 0; i < recursionLevel; i++)
        {
            std::vector<Mesh::Face> faces2;
            for (const auto& tri : mFaces)
            {
                // replace triangle by 4 triangles
                size_t a = GetMiddlePoint(tri[0], tri[1]);
                size_t b = GetMiddlePoint(tri[1], tri[2]);
                size_t c = GetMiddlePoint(tri[2], tri[0]);

                faces2.push_back({ tri[0], a, c});
                faces2.push_back({ tri[1], b, a});
                faces2.push_back({ tri[2], c, b});
                faces2.push_back({ a, b, c });
            }
            mFaces = faces2;
        }
    }

    size_t Icosphere::GetMiddlePoint(size_t p1, size_t p2)
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
        const Vertex& point1 = mVertices[p1];
        const Vertex& point2 = mVertices[p2];
        Vertex middle = {
            glm::normalize((point1.position + point2.position) / 2.0f),
            glm::normalize((point1.normal + point2.normal) / 2.0f),
            {}
        };
        size_t index = mVertices.size();
        mVertices.push_back(middle);
        mMiddlePointCache[key] = index;
        return index;
    }


    // Create initial icosahedron
    void Icosphere::CreateIcosahedron()
    {
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio

        mVertices = {
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

        mFaces = {
            { 0, 11, 5 }, 
            { 0, 5, 1 }, 
            { 0, 1, 7 }, 
            { 0, 7, 10 }, 
            { 0, 10, 11 },

            { 1, 5, 9 }, 
            { 5, 11, 4 }, 
            { 11, 10, 2 }, 
            { 10, 7, 6 },
            { 7, 1, 8 }, 

            { 3, 9, 4 }, 
            { 3, 4, 2 }, 
            { 3, 2, 6 }, 
            { 3, 6, 8 }, 
            { 3, 8, 9 },

            { 4, 9, 5 },
            { 2, 4, 11 },
            { 6, 2, 10 },
            { 8, 6, 7 },
            { 9, 8, 1 },
        };
    }

    Icosphere::Icosphere(const size_t subdivisions)
    {
        CreateIcosahedron();
        RefineTriangles(subdivisions);
    }
}
