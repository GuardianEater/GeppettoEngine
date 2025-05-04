/*****************************************************************//**
 * \file   RelationSystem.hpp
 * \brief  system that handles relations between entities, 
 *         for example when a parent moves and its children also move
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
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
        void UpdateRecursive(const Gep::Entity& parent);
    };
}

