/*****************************************************************//**
 * \file   CollisionResource.hpp
 * \brief  vaious functions used for collision detection
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   April 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

#include "Shapes.hpp"
#include "EngineManager.hpp"

#include "glm/glm.hpp"

namespace Client
{
    class CollisionResource
    {
    public:
        // returns a list of entities that are hit by the ray
        std::vector<Gep::Entity> RayCast(Gep::EngineManager& em, const Gep::Ray & ray);
    };
}
