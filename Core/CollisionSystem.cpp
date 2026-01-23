/*****************************************************************//**
 * \file   CollisionSystem.cpp
 * \brief  system that detects collisions
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
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
        //std::vector<SphereEntity> sphereEntities;

        //mManager.ForEachArchetype([&](Gep::Entity entity, Transform& t, SphereCollider& s)
        //{
        //    float radius = std::max({ t.world.scale.x, t.world.scale.y, t.world.scale.z }) / 2.0f;
        //    float minX = t.world.position.x - radius;
        //    float maxX = t.world.position.x + radius;

        //    sphereEntities.push_back({ minX, maxX, radius, entity, &t, &s });
        //});

        //std::sort(sphereEntities.begin(), sphereEntities.end(), [](const auto& a, const auto& b) 
        //{
        //    return a.xMin < b.xMin;
        //});

        // Sphere-Sphere collisions
        //for (size_t i = 0; i < sphereEntities.size(); ++i)
        //{
        //    for (size_t j = i + 1; j < sphereEntities.size(); ++j)
        //    {
        //        if (sphereEntities[j].xMin > sphereEntities[i].xMax)
        //            break;

        //        SphereEntity& view0 = sphereEntities[i];
        //        SphereEntity& view1 = sphereEntities[j];

        //        if (Gep::SphereSphere({ view0.transform->world.position, view0.radius },
        //                              { view1.transform->world.position, view1.radius }))
        //        {
        //            ////Gep::Log::Info("Collision detected between entity ", view0.entity, " and entity ", view1.entity);
        //            //mManager.SignalEvent(Gep::Event::CollisionStay{});
        //        }
        //    }
        //}

        //std::vector<Gep::Cube> cubes;

        //mManager.ForEachArchetype([&](Gep::Entity entity, Transform& t, CubeCollider& c)
        //{
        //    glm::mat3 axes = glm::mat3_cast(t.world.rotation);

        //    cubes.push_back({ t.world.position, t.world.scale * 0.5f, t.world.rotation, axes });
        //});

        //// Cube-Cube collisions
        //for (size_t i = 0; i < cubes.size(); ++i)
        //{
        //    for (size_t j = i + 1; j < cubes.size(); ++j)
        //    {
        //        const Gep::Cube& cube0 = cubes[i];
        //        const Gep::Cube& cube1 = cubes[j];

        //        if (Gep::CubeCube(cube0, cube1))
        //        {
        //            //Gep::Log::Trace("Collision detected between 2 cubes!");
        //            //mManager.SignalEvent(Gep::Event::CollisionStay{});
        //        }
        //    }
        //}

        //// Sphere-Cube collisions
        //for (const Gep::Cube& cube : cubes)
        //{
        //    for (const SphereEntity& view1 : sphereEntities)
        //    {
        //        if (Gep::CubeSphere(cube, { view1.transform->world.position, view1.radius }))
        //        {
        //            //Gep::Log::Info("Collision detected between cube and sphere!");
        //            //mManager.SignalEvent(Gep::Event::CollisionStay{});
        //        }
        //    }
        //}
    }
}

