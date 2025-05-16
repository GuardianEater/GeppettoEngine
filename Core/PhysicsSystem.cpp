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
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Transform>>(this, &PhysicsSystem::OnTransformEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<RigidBody>>(this, &PhysicsSystem::OnRigidBodyEditorRender);
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

    void PhysicsSystem::FrameEnd()
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform>();

        // make this unsequenced
        std::for_each(entities.begin(), entities.end(), [&](Gep::Entity entity)
        {
            Transform& transform = mManager.GetComponent<Transform>(entity);
            transform.previousPosition = transform.position;
            transform.previousRotation = transform.rotation;
            transform.previousScale = transform.scale;
        });
    }

    void PhysicsSystem::OnTransformEditorRender(const Gep::Event::ComponentEditorRender<Transform>& event)
    {
        Transform& transform = event.component;

        ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
        ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f, 0.0f, Gep::num_max<float>());
        ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.1f, 0.0f, 360.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_WrapAround);
    }

    void PhysicsSystem::OnRigidBodyEditorRender(const Gep::Event::ComponentEditorRender<RigidBody>& event)
    {
        RigidBody& rigidBody = event.component;

        ImGui::DragFloat3("Velocity", &rigidBody.velocity.x, 0.1f);
        ImGui::DragFloat3("Acceleration", &rigidBody.acceleration.x, 0.1f);
        ImGui::DragFloat3("Angular Velocity", &rigidBody.rotationalVelocity.x, 0.1f);
        ImGui::DragFloat3("Angular Velocity", &rigidBody.rotationalAcceleration.x, 0.1f);
    }
}

