/*****************************************************************//**
 * \file   SphereMesh.hpp
 * \brief  simple algorithm to generate a circular mesh
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <Mesh.hpp>
#include <cmath>

namespace Gep
{
    class SphereMesh : public Mesh
    {
    public:
        SphereMesh(const size_t size_m, const size_t size_n);

    private:
        const double PI = 3.14159265359;
    };
}