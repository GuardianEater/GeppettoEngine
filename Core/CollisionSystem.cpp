/*****************************************************************//**
 * \file   CollisionSystem.cpp
 * \brief  system that detects collisions
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"
#include "CollisionSystem.hpp"
#include "SphereCollider.hpp"
#include "Transform.hpp"
#include "EngineManager.hpp"
#include "Logger.hpp"

namespace Client
{
    CollisionSystem::CollisionSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    void CollisionSystem::Update(float dt)
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform, SphereCollider>();
        for (Gep::Entity entity : entities)
        {
            Transform& transform = mManager.GetComponent<Transform>(entity);
            SphereCollider& collider = mManager.GetComponent<SphereCollider>(entity);
            // check for collisions
            for (Gep::Entity otherEntity : entities)
            {
                if (entity == otherEntity)
                    continue;

                Transform& otherTransform = mManager.GetComponent<Transform>(otherEntity);
                SphereCollider& otherCollider = mManager.GetComponent<SphereCollider>(otherEntity);

                // check for collision
                float distance = glm::distance(transform.position, otherTransform.position);
                float sumOfRadii = collider.radius + otherCollider.radius;
                if (distance < sumOfRadii)
                {
                    Gep::Log::Info("Collision Detected!");
                }
            }
        }
    }
}

