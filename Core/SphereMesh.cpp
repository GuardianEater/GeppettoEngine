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

        mVertices.resize(size_m * (size_n - 1) + 2);

        for (size_t i = 1; i < size_n; ++i)
        {
            const float theta = PI * i / size_n;

            for (size_t j = 0; j < size_m; ++j)
            {
                const size_t index = size_m * (i - 1) + j;
                const float phi = 2 * PI * j / size_m;

                mVertices[index].normal.x = sin(theta) * cos(phi);
                mVertices[index].normal.y = sin(theta) * sin(phi);
                mVertices[index].normal.z = cos(theta);
            }
        }

        mVertices[north].normal = glm::vec3(0.0f, 0.0f, 1.0f);
        mVertices[south].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        for (size_t n = 0; n < mVertices.size(); ++n)
        {
            mVertices[n].position = mVertices[n].normal;
        }

        for (size_t i = 2; i < size_n; ++i)
        {
            for (size_t j = 0; j < size_m; ++j)
            {
                const size_t jp1 = (j + 1) % size_m;

                Face& face1 = mFaces.emplace_back();
                face1[0] = size_m * (i - 2) + j;
                face1[1] = size_m * (i - 1) + jp1;
                face1[2] = size_m * (i - 2) + jp1;

                Face& face2 = mFaces.emplace_back();
                face2[0] = size_m * (i - 2) + j;;
                face2[1] = size_m * (i - 1) + j;
                face2[2] = size_m * (i - 1) + jp1;
            }
        }

        for (size_t j = 0; j < size_m; ++j)
        {
            const size_t jp1 = (j + 1) % size_m;

            Face& face1 = mFaces.emplace_back();
            face1[0] = j;
            face1[1] = jp1;
            face1[2] = north;

            Face& face2 = mFaces.emplace_back();
            face2[0] = size_m * (size_n - 2) + j;
            face2[1] = south;
            face2[2] = size_m * (size_n - 2) + jp1;
        }

        for (size_t i = 2; i < size_n; ++i)
        {
            for (size_t j = 0; j < size_m; ++j)
            {
                const size_t jp1 = (j + 1) % size_m;

                Edge& edge1 = mEdges.emplace_back();
                edge1[0] = size_m * (i - 2) + j;
                edge1[1] = size_m * (i - 2) + jp1;

                Edge& edge2 = mEdges.emplace_back();
                edge2[0] = size_m * (i - 2) + j;
                edge2[1] = size_m * (i - 1) + jp1;

                Edge& edge3 = mEdges.emplace_back();
                edge3[0] = size_m * (i - 2) + jp1;
                edge3[1] = size_m * (i - 1) + jp1;
            }
        }

        for (size_t j = 0; j < size_m; ++j)
        {
            const size_t jp1 = (j + 1) % size_m;

            Edge& edge1 = mEdges.emplace_back();
            edge1[0] = size_m * (size_n - 2) + j;
            edge1[1] = size_m * (size_n - 2) + jp1;

            Edge& edge2 = mEdges.emplace_back();
            edge2[0] = size_m * (size_n - 2) + j;
            edge2[1] = south;

            Edge& edge3 = mEdges.emplace_back();
            edge3[0] = j;
            edge3[1] = north;
        }
    }
}

