/*****************************************************************//**
 * \file   CollisionResource.cpp
 * \brief
 *
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
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
        const std::vector<Gep::Entity>& cubeEntities = em.GetEntities<Client::Transform, Client::CubeCollider>();
        const std::vector<Gep::Entity>& sphereEntities = em.GetEntities<Client::Transform, Client::SphereCollider>();

        for (const auto& entity : cubeEntities)
        {
            const auto& transform = em.GetComponent<Client::Transform>(entity);
            const auto& collider = em.GetComponent<Client::CubeCollider>(entity);

            float t;

            if (Gep::RayCube(ray, Gep::Cube{ transform.position, collider.size, transform.rotation }, t))
            {
                hitEntities.push_back(entity);
            }
        }

        for (const auto& entity : sphereEntities)
        {
            const auto& transform = em.GetComponent<Client::Transform>(entity);
            const auto& collider = em.GetComponent<Client::SphereCollider>(entity);

            float t;

            if (Gep::RaySphere(ray, Gep::Sphere{ transform.position, collider.radius }, t))
            {
                hitEntities.push_back(entity);
            }
        }

        return hitEntities;
    }
}
