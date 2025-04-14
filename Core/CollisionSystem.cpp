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
        const std::vector<Gep::Entity>& cubeEntities = mManager.GetEntities<Transform, CubeCollider>();
        const std::vector<Gep::Entity>& sphereEntities = mManager.GetEntities<Transform, SphereCollider>();

        // Sphere-Sphere collisions
        for (size_t i = 0; i < sphereEntities.size(); ++i)
        {
            Gep::Entity entity = sphereEntities[i];
            Transform& transform = mManager.GetComponent<Transform>(entity);
            SphereCollider& collider = mManager.GetComponent<SphereCollider>(entity);

            for (size_t j = i + 1; j < sphereEntities.size(); ++j)
            {
                Gep::Entity otherEntity = sphereEntities[j];
                Transform& otherTransform = mManager.GetComponent<Transform>(otherEntity);
                SphereCollider& otherCollider = mManager.GetComponent<SphereCollider>(otherEntity);

                if (Gep::SphereSphere(Gep::Sphere{ transform.position, std::max({transform.scale.x, transform.scale.y, transform.scale.z}) / 2.0f },
                    Gep::Sphere{ otherTransform.position, std::max({otherTransform.scale.x, otherTransform.scale.y, otherTransform.scale.z}) / 2.0f }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", entity, " and entity ", otherEntity);
                }
            }
        }

        // Cube-Cube collisions
        for (size_t i = 0; i < cubeEntities.size(); ++i)
        {
            Gep::Entity entity = cubeEntities[i];
            Transform& transform = mManager.GetComponent<Transform>(entity);
            CubeCollider& collider = mManager.GetComponent<CubeCollider>(entity);

            for (size_t j = i + 1; j < cubeEntities.size(); ++j)
            {
                Gep::Entity otherEntity = cubeEntities[j];
                Transform& otherTransform = mManager.GetComponent<Transform>(otherEntity);
                CubeCollider& otherCollider = mManager.GetComponent<CubeCollider>(otherEntity);

                if (Gep::CubeCube(Gep::Cube{ transform.position, transform.scale / 2.0f, transform.rotation },
                    Gep::Cube{ otherTransform.position, otherTransform.scale / 2.0f, otherTransform.rotation }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", entity, " and entity ", otherEntity);
                }
            }
        }

        // Sphere-Cube collisions
        for (Gep::Entity entity : sphereEntities)
        {
            Transform& transform = mManager.GetComponent<Transform>(entity);
            SphereCollider& collider = mManager.GetComponent<SphereCollider>(entity);

            for (Gep::Entity otherEntity : cubeEntities)
            {
                Transform& otherTransform = mManager.GetComponent<Transform>(otherEntity);
                CubeCollider& otherCollider = mManager.GetComponent<CubeCollider>(otherEntity);

                if (Gep::CubeSphere(Gep::Cube{ otherTransform.position, otherTransform.scale / 2.0f, otherTransform.rotation },
                    Gep::Sphere{ transform.position, std::max({transform.scale.x, transform.scale.y, transform.scale.z}) / 2.0f }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", entity, " and entity ", otherEntity);
                }
            }
        }
    }
}

