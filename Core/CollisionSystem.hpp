/*****************************************************************//**
 * \file   CollisionSystem.hpp
 * \brief  Uses various collider components to detect collisions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "ISystem.hpp"

namespace Client
{
    class CollisionSystem : public Gep::ISystem
    {
    public:
        CollisionSystem(Gep::EngineManager& em);

    private:
        void Update(float dt) override;
    };
}
