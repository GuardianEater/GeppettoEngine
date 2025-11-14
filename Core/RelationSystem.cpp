/*****************************************************************//**
 * \file   RelationSystem.cpp
 * \brief  handles relations between entities
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "RelationSystem.hpp"
#include "EngineManager.hpp"
#include "Transform.hpp"

namespace Client
{
    void RelationSystem::Update(float dt)
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform>();

        for (Gep::Entity entity : entities)
        {
            // only root entities
            if (!mManager.HasParent(entity))
            {
                UpdateRecursive(entity);
            }
        }
        
    }

    void RelationSystem::UpdateRecursive(const Gep::Entity& parent) 
    {

    }


}
