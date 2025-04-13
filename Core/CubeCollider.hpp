/*****************************************************************//**
 * \file   CubeCollider.hpp
 * \brief  Component that represents a cube collider
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   April 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

namespace Client
{
    struct CubeCollider
    {
        glm::vec3 size{ 1.0f, 1.0f, 1.0f }; // size of the cube
    };
}
