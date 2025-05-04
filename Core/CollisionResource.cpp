/*****************************************************************//**
 * \file   CollisionResource.cpp
 * \brief
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   April 2025
 *********************************************************************/

#include "pch.hpp"

#include "CollisionResource.hpp"
#include "CollisionChecking.hpp"

#include "SphereCollider.hpp"
#include "CubeCollider.hpp"
#include "Transform.hpp"

namespace Client
{
    std::vector<Gep::Entity> CollisionResource::RayCast(Gep::EngineManager& em, const Gep::Ray& ray)
    {
        std::vector<Gep::Entity> hitEntities;

        std::vector<std::pair<float, Gep::Entity>> hits;

        em.ForEachArchetype<Client::Transform, Client::CubeCollider>([&](Gep::Entity entity, Client::Transform& transform, Client::CubeCollider& collider) 
        {
            float t;
            if (Gep::RayCube(ray, Gep::Cube{ transform.position, transform.scale * 0.5f, transform.rotation }, t))
            {
                hits.emplace_back(t, entity);
            }
        });

        em.ForEachArchetype<Client::Transform, Client::SphereCollider>([&](Gep::Entity entity, Client::Transform& transform, Client::SphereCollider& collider)
        {
            float t;
            if (Gep::RaySphere(ray, Gep::Sphere{ transform.position, std::max({transform.scale.x, transform.scale.y, transform.scale.z}) * 0.5f }, t))
            {
                hits.emplace_back(t, entity);
            }        
        });

        std::sort(hits.begin(), hits.end(), [](const auto& a, const auto& b) 
        {
            return a.first < b.first;
        });

        hitEntities.reserve(hits.size());
        for (const auto& hit : hits)
        {
            hitEntities.push_back(hit.second);
        }

        return hitEntities;
    }
}
