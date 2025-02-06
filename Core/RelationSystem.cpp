/*****************************************************************//**
 * \file   RelationSystem.cpp
 * \brief  handles relations between entities
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
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
        const std::vector<Gep::Entity>& entities = mManager.GetEntities();

        for (Gep::Entity entity : entities)
        {
            if (mManager.HasParent(entity))
            {
                Transform& parentTransform = mManager.GetComponent<Transform>(mManager.GetParent(entity));
                Transform& childTransform = mManager.GetComponent<Transform>(entity);

                // move the child by the delta of the parent

                glm::vec3 parentDelta = parentTransform.position - parentTransform.previousPosition;

                childTransform.position += parentDelta;
            }
        }
        
    }
}
