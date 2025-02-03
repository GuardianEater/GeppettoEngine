/*****************************************************************//**
 * \file   RelationSystem.hpp
 * \brief  system that handles relations between entities, 
 *         for example when a parent moves and its children also move
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "ISystem.hpp"

#pragma once
namespace Client
{
    class RelationSystem : public Gep::ISystem
    {
    public:
        RelationSystem(Gep::EngineManager& em)
            : ISystem(em)
        {}

        void Update(float dt) override;
    };
}

