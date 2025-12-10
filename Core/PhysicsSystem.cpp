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
#include "Conversion.h"

// std
#include <iostream>

namespace Client
{
    PhysicsSystem::PhysicsSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Transform>>(this, &PhysicsSystem::OnTransformEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<RigidBody>>(this, &PhysicsSystem::OnRigidBodyEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Spring>>(this, &PhysicsSystem::OnSpringEditorRender);

        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<RigidBody>>(this, &PhysicsSystem::OnRigidBodyAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Spring>>(this, &PhysicsSystem::OnSpringAdded);
    }

    PhysicsSystem::~PhysicsSystem() = default;

    void PhysicsSystem::Update(float dt)
    {
        ApplySpringForces();
        Integrate(dt);

        DrawSprings();
    }

    void PhysicsSystem::FrameEnd()
    {
        mManager.ForEachArchetype<Transform>([&](Gep::Entity entity, Transform& t)
        {
            t.previousWorld = t.world;
        });
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
        RigidBody& rb = event.component;

        ImGui::DragFloat3("Velocity", &rb.linearVelocity.x, 0.1f);
        ImGui::DragFloat3("Angular Velocity", &rb.angularVelocity.x, 0.1f);

        if (ImGui::TreeNode("Testing"))
        {
            static glm::vec3 force{};
            static glm::vec3 torque{};
            static glm::vec3 point{};

            ImGui::DragFloat3("Force", &force.x);
            ImGui::DragFloat3("Position", &point.x);
            ImGui::DragFloat3("Torque", &torque.x);

            if (ImGui::Button("Apply Force"))
            {
                rb.ApplyForce(force);
            }

            if (ImGui::Button("Apply Torque"))
            {
                rb.ApplyTorque(torque);
            }

            if (mManager.HasComponent<Transform>(event.entity))
            {
                Transform& t = mManager.GetComponent<Transform>(event.entity);
                if (ImGui::Button("Apply Force at Point"))
                {
                    rb.ApplyForceAtPoint(t, force, point);
                }
            }

            if (ImGui::Button("Stop"))
            {
                rb.angularVelocity = {};
                rb.linearVelocity = {};
            }

            ImGui::TreePop();
        }


    }

    void PhysicsSystem::OnRigidBodyAdded(const Gep::Event::ComponentAdded<RigidBody>& event)
    {

    }

    void PhysicsSystem::Integrate_RungeKutta4(RigidBody& rb, Transform& t, float dt) const
    {
    }

    void PhysicsSystem::Integrate_ExplicitEuler(RigidBody& rb, Transform& t, const Transform* parentT, float dt) const
    {
        glm::vec3 a = rb.LinearAcceleration();
        glm::vec3 alpha = rb.AngularAcceleration(t);

        t.world.position += rb.linearVelocity * dt;
        rb.linearVelocity += a * dt;

        glm::quat dq = Gep::Derivative(t.world.rotation, rb.angularVelocity);
        t.world.rotation += dq * dt;
        t.world.rotation = glm::normalize(t.world.rotation);

        // recalculate local
        if (parentT)
            t.local = Gep::Inverse(parentT->world) * t.world;
        else
            t.local = t.world;

        rb.angularVelocity += alpha * dt;

        rb.ClearAccumulators();
    }

    void PhysicsSystem::Integrate_SemiImplicitEuler(RigidBody& rb, Transform& t, float dt) const
    {
    }

    void PhysicsSystem::Integrate_Verlet(RigidBody& rb, Transform& t, float dt) const
    {
    }

    constexpr float kMinSpringStiffness = 0.0f;
    constexpr float kMaxSpringStiffness = 1.0e5f; // tweak to taste

    void PhysicsSystem::ApplySpringForces()
    {
        mManager.ForEachArchetype<Spring>([&](Gep::Entity e, Spring& spring)
        {
            if (spring.stiffness <= 0.0f || !spring.startEntity.IsValid())
                 return;

            Gep::Entity startEntity = mManager.FindEntity(spring.startEntity);
            Gep::Entity endEntity = mManager.FindEntity(spring.endEntity);

            // confirms both entities exist
            if (!mManager.EntityExists(startEntity) || 
                !mManager.EntityExists(endEntity))
                return;
            
            // confirms both entitiies have transforms
            if (!mManager.HasComponent<Transform>(startEntity) || 
                !mManager.HasComponent<Transform>(endEntity)) 
                return;
            
            // gets both transforms
            Transform& startTransform = mManager.GetComponent<Transform>(startEntity);
            Transform& endTransform   = mManager.GetComponent<Transform>(endEntity);

            glm::vec3 x = startTransform.world.position - endTransform.world.position;
            float distance = glm::length(x);
            if (distance <= 1e-3f)
                return;

            glm::vec3 direction = x / distance;
            float displacement = distance - std::max(0.0f, spring.restLength);
            glm::vec3 springForce = -spring.stiffness * displacement * direction;

            glm::vec3 endVelocity{ 0.0f };
            RigidBody* endRb = nullptr;
            if (mManager.HasComponent<RigidBody>(endEntity))
            {
                endRb = &mManager.GetComponent<RigidBody>(endEntity);
                endVelocity = endRb->linearVelocity;
            }

            glm::vec3 startVelocity{ 0.0f };
            RigidBody * startRb = nullptr;
            if (mManager.HasComponent<RigidBody>(startEntity))
            {
                startRb = &mManager.GetComponent<RigidBody>(startEntity);
                startVelocity = startRb->linearVelocity;
            }
            
            glm::vec3 dampingForce{ 0.0f };
            if (spring.damping > 0.0f)
            {
                glm::vec3 relativeVelocity = startVelocity - endVelocity;
                dampingForce = -spring.damping * relativeVelocity;
            }

            glm::vec3 totalForce = springForce + dampingForce;

            if (startRb)
                startRb->ApplyForce(totalForce);
            if (endRb)
                endRb->ApplyForce(-totalForce);
        });
    }

    void PhysicsSystem::Integrate(float dt)
    {
        mManager.ForEachArchetype<RigidBody, Transform>([&](Gep::Entity e, RigidBody& rb, Transform& t)
        {
            Gep::Entity p = mManager.GetParent(e);

            // if this entities parent exists get its transform
            const Transform* parentT = nullptr;
            if (mManager.EntityExists(p))
                parentT = &mManager.GetComponent<Transform>(p);

            Integrate_ExplicitEuler(rb, t, parentT, dt);
        });
    }

    void PhysicsSystem::DrawSprings()
    {

    }

    void PhysicsSystem::OnSpringEditorRender(const Gep::Event::ComponentEditorRender<Spring>&event)
    {
        Spring& spring = event.component;
        EditorResource& editor = mManager.GetResource<EditorResource>();

        Gep::Entity startEntity = mManager.FindEntity(spring.startEntity);
        Gep::Entity endEntity = mManager.FindEntity(spring.endEntity);

        const ImVec4 validColor{ 0.2f, 0.8f, 0.2f, 1.0f };
        const ImVec4 invalidColor{ 0.9f, 0.2f, 0.2f, 1.0f };
        
        ImGui::BeginGroup(); // group for drag drop
        bool startValid = false;

        // Check entity existence and required components, but avoid early returns.
        if (!mManager.EntityExists(startEntity))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing Start Entity");
        }
        else
        {
            bool hasTransform = mManager.HasComponent<Transform>(startEntity);

            ImGui::Text("Following Entity:");
            ImGui::SameLine();
            // display yellow if its missing a needed component
            ImGui::TextColored(hasTransform ? validColor : invalidColor, mManager.GetName(startEntity).c_str());

            // display the missing components
            if (!hasTransform)
                ImGui::TextColored(invalidColor, "Start Entity Missing Transform");

            startValid = (hasTransform);
        }

        ImGui::EndGroup();

        // drag drop for the entire group
        editor.EntityDragDropTarget([&](Gep::Entity e)
        {
            event.component.startEntity = mManager.GetUUID(e);
        });

        // if the needed checks failed dont continue with the ui


        ImGui::BeginGroup(); // group for drag drop
        bool endValid = false;

        // Check entity existence and required components, but avoid early returns.
        if (!mManager.EntityExists(endEntity))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Missing End Entity");
        }
        else
        {
            bool hasTransform = mManager.HasComponent<Transform>(endEntity);

            ImGui::Text("Following Entity:");
            ImGui::SameLine();
            // display yellow if its missing a needed component
            ImGui::TextColored(hasTransform ? validColor : invalidColor, mManager.GetName(endEntity).c_str());

            // display the missing components
            if (!hasTransform)
                ImGui::TextColored(invalidColor, "End Entity Missing Transform");

            endValid = (hasTransform);
        }

        ImGui::EndGroup();

        // drag drop for the entire group
        editor.EntityDragDropTarget([&](Gep::Entity e)
        {
            event.component.endEntity = mManager.GetUUID(e);
        });

        if (!endValid) return;
        if (!startValid) return;
        
        Transform& startTransform = mManager.GetComponent<Transform>(startEntity);
        Transform& endTransform   = mManager.GetComponent<Transform>(endEntity);

        if (ImGui::Button("Match Rest Length"))
        {
            spring.restLength = glm::distance(startTransform.world.position, endTransform.world.position);
        }
        
        ImGui::DragFloat("Rest Length", &spring.restLength);
        ImGui::DragFloat("Stiffness", &spring.stiffness, 0.1f);
        ImGui::DragFloat("Damping", &spring.damping, 0.01f);
        
        spring.restLength = std::max(0.0f, spring.restLength);
        spring.stiffness = std::max(0.0f, spring.stiffness);
        spring.damping = std::max(0.0f, spring.damping);
    }

    void PhysicsSystem::OnSpringAdded(const Gep::Event::ComponentAdded<Spring>& event)
    {

    }
}

