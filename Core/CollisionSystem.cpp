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

    struct SphereEntity
    {
        float xMin{};
        float xMax{};
        float radius{};
        Gep::Entity entity;
        Transform* transform;
        SphereCollider* collider;
    };

    struct CubeEntity
    {
        Gep::Entity entity;
        Transform* transform;
        CubeCollider* collider;
    };

    void CollisionSystem::Update(float dt)
    {
        std::vector<SphereEntity> sphereEntities;

        mManager.ForEachArchetype<Transform, SphereCollider>([&](Gep::Entity entity, Transform& t, SphereCollider& s)
        {
            float radius = std::max({ t.scale.x, t.scale.y, t.scale.z }) / 2.0f;
            float minX = t.position.x - radius;
            float maxX = t.position.x + radius;

            sphereEntities.push_back({ minX, maxX, radius, entity, &t, &s });
        });

        std::sort(sphereEntities.begin(), sphereEntities.end(), [](const auto& a, const auto& b) 
        {
            return a.xMin < b.xMin;
        });

        // Sphere-Sphere collisions
        for (size_t i = 0; i < sphereEntities.size(); ++i)
        {
            for (size_t j = i + 1; j < sphereEntities.size(); ++j)
            {
                if (sphereEntities[j].xMin > sphereEntities[i].xMax)
                    break;

                SphereEntity& view0 = sphereEntities[i];
                SphereEntity& view1 = sphereEntities[j];

                if (Gep::SphereSphere({ view0.transform->position, view0.radius },
                                      { view1.transform->position, view1.radius }))
                {
                    Gep::Log::Info("Collision detected between entity ", view0.entity, " and entity ", view1.entity);
                }
            }
        }

        std::vector<CubeEntity> cubeEntities;

        mManager.ForEachArchetype<Transform, CubeCollider>([&](Gep::Entity entity, Transform& t, CubeCollider& c)
        {
            cubeEntities.push_back({ entity, &t, &c });
        });

        // Cube-Cube collisions
        for (size_t i = 0; i < cubeEntities.size(); ++i)
        {
            for (size_t j = i + 1; j < cubeEntities.size(); ++j)
            {
                const CubeEntity& view0 = cubeEntities[i];
                const CubeEntity& view1 = cubeEntities[j];


                if (Gep::CubeCube({ view0.transform->position, view0.transform->scale * 0.5f, view0.transform->rotation },
                                  { view1.transform->position, view1.transform->scale * 0.5f, view1.transform->rotation }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", view0.entity, " and entity ", view1.entity);
                }
            }
        }

        // Sphere-Cube collisions
        for (const CubeEntity& view0 : cubeEntities)
        {
            for (const SphereEntity& view1 : sphereEntities)
            {
                if (Gep::CubeSphere({view0.transform->position, view0.transform->scale * 0.5f, view0.transform->rotation },
                                    { view1.transform->position, view1.radius }))
                {
                    // collision detected
                    Gep::Log::Info("Collision detected between entity ", view0.entity, " and entity ", view1.entity);
                }
            }
        }
    }
}

