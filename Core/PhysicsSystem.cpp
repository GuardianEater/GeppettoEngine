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
#include <ModelComponent.hpp>

// resouce
#include <EditorResource.hpp>

// engine
#include <EngineManager.hpp>
#include <Events.hpp>

// std
#include <iostream>

namespace Client
{
    PhysicsSystem::PhysicsSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Transform>>(this, &PhysicsSystem::OnTransformEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<RigidBody>>(this, &PhysicsSystem::OnRigidBodyEditorRender);

        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<RawModelComponent>>(this, &PhysicsSystem::OnRawModelAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<RigidBody>>(this, &PhysicsSystem::OnRigidBodyAdded);
    }

    PhysicsSystem::~PhysicsSystem() = default;

    void PhysicsSystem::Update(float dt)
    {
        mManager.ForEachArchetype<RigidBody, Transform>([&](Gep::Entity entity, RigidBody& rb, Transform& t)
        {

        });
    }

    void PhysicsSystem::FrameEnd()
    {

    }

    void PhysicsSystem::OnTransformEditorRender(const Gep::Event::ComponentEditorRender<Transform>& event)
    {
        Transform& transform = event.component;
        EditorResource& er = mManager.GetResource<EditorResource>();

        ImGui::DragFloat3("Position", &transform.local.position.x, 0.1f);
        ImGui::DragFloat3("Scale", &transform.local.scale.x, 0.1f, 0.0f, Gep::num_max<float>());
        ImGui::DragFloat4("Rotation", &transform.local.rotation.x, 0.1f);

        ImGui::BeginDisabled();
        ImGui::DragFloat3("World Position", glm::value_ptr(transform.world.position), 0.1f);
        ImGui::DragFloat3("World Scale", glm::value_ptr(transform.world.scale), 0.1f, 0.0f, Gep::num_max<float>());
        ImGui::DragFloat4("World Rotation", glm::value_ptr(transform.world.rotation), 0.1f);
        ImGui::EndDisabled();
    }

    void PhysicsSystem::OnRigidBodyEditorRender(const Gep::Event::ComponentEditorRender<RigidBody>& event)
    {
        RigidBody& rigidBody = event.component;

        ImGui::DragFloat3("Velocity", &rigidBody.velocity.x, 0.1f);
        ImGui::DragFloat3("Acceleration", &rigidBody.acceleration.x, 0.1f);
        ImGui::DragFloat3("Angular Velocity", &rigidBody.rotationalVelocity.x, 0.1f);
        ImGui::DragFloat3("Angular Acceleration", &rigidBody.rotationalAcceleration.x, 0.1f);
    }

    void PhysicsSystem::OnRigidBodyAdded(const Gep::Event::ComponentAdded<RigidBody>& event)
    {

    }

    void PhysicsSystem::OnRawModelAdded(const Gep::Event::ComponentAdded<RawModelComponent>& event)
    {

    }
}

