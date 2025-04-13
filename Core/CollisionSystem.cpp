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
#include "CubeCollider.hpp"
#include "Transform.hpp"
#include "EngineManager.hpp"
#include "Logger.hpp"
#include "CollisionChecking.hpp"

namespace Client
{
    CollisionSystem::CollisionSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    void CollisionSystem::Update(float dt)
    {
        // sphere sphere
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

                if (Gep::SphereSphere(Gep::Sphere{ transform.position, collider.radius },
                                      Gep::Sphere{ otherTransform.position, otherCollider.radius }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", entity, " and entity ", otherEntity);
                }
            }

        }

        // cube cube
        const std::vector<Gep::Entity>& cubeEntities = mManager.GetEntities<Transform, CubeCollider>();
        for (Gep::Entity otherEntity : cubeEntities)
        {
            Transform& otherTransform = mManager.GetComponent<Transform>(otherEntity);
            CubeCollider& otherCollider = mManager.GetComponent<CubeCollider>(otherEntity);
            
            // check for collisions
            for (Gep::Entity entity : cubeEntities)
            {
                if (entity == otherEntity)
                    continue;

                Transform& transform = mManager.GetComponent<Transform>(entity);
                CubeCollider& collider = mManager.GetComponent<CubeCollider>(entity);

                if (Gep::CubeCube(Gep::Cube{ transform.position, collider.size, transform.rotation },
                                  Gep::Cube{ otherTransform.position, otherCollider.size , otherTransform.rotation}))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", entity, " and entity ", otherEntity);
                }
            }
        }

        // sphere cube
        const std::vector<Gep::Entity>& sphereEntities = mManager.GetEntities<Transform, SphereCollider>();
        for (Gep::Entity entity : sphereEntities)
        {
            Transform& transform = mManager.GetComponent<Transform>(entity);
            SphereCollider& collider = mManager.GetComponent<SphereCollider>(entity);
            // check for collisions
            for (Gep::Entity otherEntity : cubeEntities)
            {
                Transform& otherTransform = mManager.GetComponent<Transform>(otherEntity);
                CubeCollider& otherCollider = mManager.GetComponent<CubeCollider>(otherEntity);
                if (Gep::CubeSphere(Gep::Cube{ otherTransform.position, otherCollider.size, otherTransform.rotation },
                    Gep::Sphere{ transform.position, collider.radius }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", entity, " and entity ", otherEntity);
                }
            }
        }
    }
}

