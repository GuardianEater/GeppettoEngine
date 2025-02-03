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
        // Retrieve all entities with Transform and Parent components.

    }


    void RelationSystem::UpdateEntity(Gep::Entity entity, const glm::vec3& parentDelta)
    {

    }

}
