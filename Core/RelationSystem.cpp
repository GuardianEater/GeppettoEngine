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
        Transform& parentTransform = mManager.GetComponent<Transform>(parent);
        glm::vec3 parentDelta = parentTransform.position - parentTransform.previousPosition;

        glm::quat parentQuat     = glm::quat(glm::radians(parentTransform.rotation));
        glm::quat prevParentQuat = glm::quat(glm::radians(parentTransform.previousRotation));
        glm::quat deltaQuat      = parentQuat * glm::inverse(prevParentQuat);

        // Retrieve the parent's children.
        const std::vector<Gep::Entity>& children = mManager.GetChildren(parent);
        for (const Gep::Entity& child : children) 
        {
            Transform& childTransform = mManager.GetComponent<Transform>(child);

            // Compute the child's offset relative to the parent's previous position.
            glm::vec3 offset = childTransform.position - parentTransform.previousPosition;
            offset = deltaQuat * offset;

            childTransform.position = parentTransform.position + offset;

            glm::quat childQuat = glm::quat(glm::radians(childTransform.rotation));
            glm::quat newChildQuat = deltaQuat * childQuat;
            
            childTransform.rotation = glm::degrees(glm::eulerAngles(newChildQuat));

            // Recursively update nested children.
            UpdateRecursive(child);
        }
    }


}
