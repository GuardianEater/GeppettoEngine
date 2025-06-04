/*****************************************************************//**
 * \file   SphereMesh.cpp
 * \brief  simple algorithm to generate a circular mesh
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "pch.hpp"

#include "SphereMesh.hpp"

namespace Gep
{
    SphereMesh::SphereMesh(const size_t size_m, const size_t size_n) : Mesh()
    {
        const size_t north = size_m * (size_n - 1);
        const size_t south = size_m * (size_n - 1) + 1;

        vertices.resize(size_m * (size_n - 1) + 2);

        for (size_t i = 1; i < size_n; ++i)
        {
            const float theta = PI * i / size_n;

            for (size_t j = 0; j < size_m; ++j)
            {
                const size_t index = size_m * (i - 1) + j;
                const float phi = 2 * PI * j / size_m;

                vertices[index].normal.x = sin(theta) * cos(phi);
                vertices[index].normal.y = sin(theta) * sin(phi);
                vertices[index].normal.z = cos(theta);
            }
        }

        vertices[north].normal = glm::vec3(0.0f, 0.0f, 1.0f);
        vertices[south].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        for (size_t n = 0; n < vertices.size(); ++n)
        {
            vertices[n].position = vertices[n].normal;
        }

        for (size_t i = 2; i < size_n; ++i)
        {
            for (size_t j = 0; j < size_m; ++j)
            {
                const size_t jp1 = (j + 1) % size_m;

                // First triangle
                indices.push_back(size_m * (i - 2) + j);
                indices.push_back(size_m * (i - 1) + jp1);
                indices.push_back(size_m * (i - 2) + jp1);

                // Second triangle
                indices.push_back(size_m * (i - 2) + j);
                indices.push_back(size_m * (i - 1) + j);
                indices.push_back(size_m * (i - 1) + jp1);
            }
        }

        // Caps (north and south poles)
        for (size_t j = 0; j < size_m; ++j)
        {
            const size_t jp1 = (j + 1) % size_m;

            // North cap
            indices.push_back(j);
            indices.push_back(jp1);
            indices.push_back(north);

            // South cap
            indices.push_back(size_m * (size_n - 2) + j);
            indices.push_back(south);
            indices.push_back(size_m * (size_n - 2) + jp1);
        }

        //for (size_t i = 2; i < size_n; ++i)
        //{
        //    for (size_t j = 0; j < size_m; ++j)
        //    {
        //        const size_t jp1 = (j + 1) % size_m;

        //        Edge& edge1 = mEdges.emplace_back();
        //        edge1[0] = size_m * (i - 2) + j;
        //        edge1[1] = size_m * (i - 2) + jp1;

        //        Edge& edge2 = mEdges.emplace_back();
        //        edge2[0] = size_m * (i - 2) + j;
        //        edge2[1] = size_m * (i - 1) + jp1;

        //        Edge& edge3 = mEdges.emplace_back();
        //        edge3[0] = size_m * (i - 2) + jp1;
        //        edge3[1] = size_m * (i - 1) + jp1;
        //    }
        //}

        //for (size_t j = 0; j < size_m; ++j)
        //{
        //    const size_t jp1 = (j + 1) % size_m;

        //    Edge& edge1 = mEdges.emplace_back();
        //    edge1[0] = size_m * (size_n - 2) + j;
        //    edge1[1] = size_m * (size_n - 2) + jp1;

        //    Edge& edge2 = mEdges.emplace_back();
        //    edge2[0] = size_m * (size_n - 2) + j;
        //    edge2[1] = south;

        //    Edge& edge3 = mEdges.emplace_back();
        //    edge3[0] = j;
        //    edge3[1] = north;
        //}

        Normalize();
    }
}

