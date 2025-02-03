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
#include "ChildComponent.hpp"
#include "ParentComponent.hpp"

namespace Client
{
    void RelationSystem::Update(float dt)
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform, Parent>();
        for (Gep::Entity entity : entities)
        {
            Parent& parent = mManager.GetComponent<Parent>(entity);
            Transform& parentTransform = mManager.GetComponent<Transform>(entity);

            glm::vec3 positionDelta = parentTransform.position - parentTransform.previousPosition;

            for (Gep::Entity childEntity : parent.children)
            {
                Transform& childTransform = mManager.GetComponent<Transform>(childEntity);
                childTransform.position += positionDelta;
            }
        }
    }
}
