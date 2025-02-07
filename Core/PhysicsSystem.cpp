/*****************************************************************//**
 * \file   PhysicsSystem.cpp
 * \brief  test physics simulation
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

// this
#include "PhysicsSystem.hpp"

 // components
#include <Transform.hpp>
#include <RigidBody.hpp>

// engine
#include <EngineManager.hpp>

// std
#include <iostream>

namespace Client
{
    PhysicsSystem::PhysicsSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    PhysicsSystem::~PhysicsSystem() = default;

    void PhysicsSystem::Update(float dt)
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform, RigidBody>();
        for (Gep::Entity entity : entities)
        {
            Transform& transform = mManager.GetComponent<Transform>(entity);
            RigidBody& rigidBody = mManager.GetComponent<RigidBody>(entity);

            transform.position += rigidBody.velocity * dt;
            rigidBody.velocity += rigidBody.acceleration * dt;

            transform.rotation += rigidBody.rotationalVelocity * dt;
            rigidBody.rotationalVelocity += rigidBody.rotationalAcceleration * dt;
        }
    }

    void PhysicsSystem::EntityDestroyed(const Gep::Event::EntityDestroyed& eventData)
    {
        std::cout << "Physics system just got the entity destroyed event" << std::endl;
    }

    void PhysicsSystem::KeyPressed(const Gep::Event::KeyPressed& eventData)
    {
        if (eventData.action == 1)
        {
            std::cout << "pressed down" << std::endl;
        }

        if (eventData.action == 0)
        {
            std::cout << "released" << std::endl;
        }

        if (eventData.action == 2)
        {
            std::cout << "held" << std::endl;
        }
    }

    void PhysicsSystem::FrameEnd()
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform>();

        // make this unsequenced
        std::for_each(std::execution::par_unseq, entities.begin(), entities.end(), [&](Gep::Entity entity)
        {
            Transform& transform = mManager.GetComponent<Transform>(entity);
            transform.previousPosition = transform.position;
            transform.previousRotation = transform.rotation;
            transform.previousScale = transform.scale;
        });
    }
}

