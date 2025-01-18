/*****************************************************************//**
 * \file   Material.hpp
 * \brief  Component for storing material data such as color or texture
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <glm.hpp>

namespace Client
{
    struct Material
    {
        glm::vec3 diff_coeff; // color
        glm::vec3 spec_coeff; // shine color
        float spec_exponent; // amount of shine
        size_t meshID;
    };
}
