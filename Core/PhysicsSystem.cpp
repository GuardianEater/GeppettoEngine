/*****************************************************************//**
 * \file   PhysicsSystem.cpp
 * \brief  test physics simulation
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

// pch
#include "pch.hpp"

// this
#include "PhysicsSystem.hpp"

 // components
#include <Transform.hpp>
#include <RigidBody.hpp>
#include <ModelComponent.hpp>

// resouce
#include "EditorResource.hpp"
#include "OpenGLRenderer.hpp"

// engine
#include "EngineManager.hpp"
#include "Events.hpp"

// help
#include "ImGuiHelp.hpp"
#include "GLMHelp.hpp"

// gtl
#include "gtl/uuid.hpp"

namespace Client
{
    PhysicsSystem::PhysicsSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
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
        // only integrate while playing
        if (mManager.IsState(Gep::EngineState::Play))
        {
            mStepAccumulator = std::min(mStepAccumulator + dt, mMaxAccumulatedTime);

            while (mStepAccumulator >= mFixedTimeStep)
            {
                Integrate(mFixedTimeStep);

                mStepAccumulator -= mFixedTimeStep;
            }
        }

        if (mRenderSprings) 
            DrawSprings();

        HandleInputs();
    }

    void PhysicsSystem::FrameEnd()
    {
        mManager.ForEachArchetype([&](Gep::Entity entity, Transform& t)
        {
            t.previousWorld = t.world;
        });
    }

    

    void PhysicsSystem::OnTransformEditorRender(const Gep::Event::ComponentEditorRender<Transform>& event)
    {
        std::span<Transform*> transforms = event.components;

        Gep::ImGui::MultiDragFloat3("Position", transforms, 
            [](Transform* t) -> float& { return t->local.position.x; },
            [](Transform* t) -> float& { return t->local.position.y; },
            [](Transform* t) -> float& { return t->local.position.z; }
        );

        Gep::ImGui::MultiDragFloat3("Scale", transforms,
            [](Transform* t) -> float& { return t->local.scale.x; },
            [](Transform* t) -> float& { return t->local.scale.y; },
            [](Transform* t) -> float& { return t->local.scale.z; }
        );

        Gep::ImGui::MultiDragFloat4("Rotation", transforms,
            [](Transform* t) -> float& { return t->local.rotation.x; },
            [](Transform* t) -> float& { return t->local.rotation.y; },
            [](Transform* t) -> float& { return t->local.rotation.z; },
            [](Transform* t) -> float& { return t->local.rotation.w; }
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool worldPositionChanged = Gep::ImGui::MultiDragFloat3("World Position", transforms,
            [](Transform* t) -> float& { return t->world.position.x; },
            [](Transform* t) -> float& { return t->world.position.y; },
            [](Transform* t) -> float& { return t->world.position.z; }
        );
        bool worldScaleChanged = Gep::ImGui::MultiDragFloat3("World Scale", transforms,
            [](Transform* t) -> float& { return t->world.scale.x; },
            [](Transform* t) -> float& { return t->world.scale.y; },
            [](Transform* t) -> float& { return t->world.scale.z; }
        );
        bool worldRotationChanged = Gep::ImGui::MultiDragFloat4("World Rotation", transforms,
            [](Transform* t) -> float& { return t->world.rotation.x; },
            [](Transform* t) -> float& { return t->world.rotation.y; },
            [](Transform* t) -> float& { return t->world.rotation.z; },
            [](Transform* t) -> float& { return t->world.rotation.w; }
        );

        if (worldPositionChanged || worldScaleChanged || worldRotationChanged)
        {
            for (size_t i = 0; i < transforms.size(); ++i)
            {
                Transform& t = *transforms[i];
                Gep::Entity e = event.entities[i];
                Gep::Entity p = mManager.GetParent(e);

                // Get parent's world transform
                if (mManager.EntityExists(p) && mManager.HasComponent<Client::Transform>(p))
                {
                    auto& pt = mManager.GetComponent<Client::Transform>(p);
                    t.local = Gep::Inverse(pt.world) * t.world;
                }
                else
                {
                    t.local = t.world;
                }
            }
        }
    }

    void PhysicsSystem::OnRigidBodyEditorRender(const Gep::Event::ComponentEditorRender<RigidBody>& event)
    {
        std::span<RigidBody*> rbs = event.components;

        Gep::ImGui::MultiDragFloat3("Velocity", rbs,
            [](RigidBody* rb) -> float& { return rb->linearVelocity.x; },
            [](RigidBody* rb) -> float& { return rb->linearVelocity.y; },
            [](RigidBody* rb) -> float& { return rb->linearVelocity.z; }
        );

        Gep::ImGui::MultiDragFloat3("Angular Velocity", rbs,
            [](RigidBody* rb) -> float& { return rb->angularVelocity.x; },
            [](RigidBody* rb) -> float& { return rb->angularVelocity.y; },
            [](RigidBody* rb) -> float& { return rb->angularVelocity.z; }
        );

        bool massChanged = Gep::ImGui::MultiDragFloat("Mass", rbs,
            [](RigidBody* rb) -> float& { return rb->mass; }
        );

        if (massChanged)
        {
            for (RigidBody* rb : rbs)
            {
                rb->mass = std::max(rb->mass, 1.0f);
                rb->invMass = 1.0f / rb->mass;
            }
        }

        // the follow tools are only available when a single rigid body is selected
        if (event.components.size() > 1)
            return;

        RigidBody& rb = *rbs[0];
        Gep::Entity e = event.entities[0];

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

            if (mManager.HasComponent<Transform>(e))
            {
                Transform& t = mManager.GetComponent<Transform>(e);
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
        struct State
        {
            glm::vec3 position;
            glm::vec3 linearVelocity;
            glm::quat rotation;
            glm::vec3 angularVelocity;
        };

        const auto evaluate = [rb](const State& s, float dtOffset, const State& d)
        {
            State state{}; 
            state.position        = s.position + d.position * dtOffset;
            state.linearVelocity  = s.linearVelocity + d.linearVelocity * dtOffset;
            state.rotation        = glm::normalize(s.rotation + d.rotation * dtOffset);
            state.angularVelocity = s.angularVelocity + d.angularVelocity * dtOffset;

            State derivative{};
            derivative.position        = state.linearVelocity;
            derivative.linearVelocity  = rb.LinearAcceleration();
            derivative.rotation        = Gep::Derivative(state.rotation, state.angularVelocity);
            derivative.angularVelocity = rb.AngularAcceleration(state.rotation);
            return derivative;
        };

        // auto lamda to get the weighted average of any types
        const auto weightedAverage = [](const auto& k1, const auto& k2, const auto& k3, const auto& k4)
        {
            return (k1 + 2.0f * (k2 + k3) + k4) / 6.0f;
        };

        const State currentState{ t.world.position, rb.linearVelocity, t.world.rotation, rb.angularVelocity };

        const State zero{};
        const State k1 = evaluate(currentState, 0.0f, zero);
        const State k2 = evaluate(currentState, 0.5f * dt, k1);
        const State k3 = evaluate(currentState, 0.5f * dt, k2);
        const State k4 = evaluate(currentState, dt, k3);

        t.world.position += weightedAverage(k1.position, k2.position, k3.position, k4.position) * dt;
        rb.linearVelocity += weightedAverage(k1.linearVelocity, k2.linearVelocity, k3.linearVelocity, k4.linearVelocity) * dt;

        glm::quat rotationDelta = weightedAverage(k1.rotation, k2.rotation, k3.rotation, k4.rotation);
        t.world.rotation = glm::normalize(t.world.rotation + rotationDelta * dt);

        rb.angularVelocity += weightedAverage(k1.angularVelocity, k2.angularVelocity, k3.angularVelocity, k4.angularVelocity) * dt;
    }

    void PhysicsSystem::Integrate_ExplicitEuler(RigidBody& rb, Transform& t, float dt) const
    {
        glm::vec3 a = rb.LinearAcceleration();
        glm::vec3 alpha = rb.AngularAcceleration(t.world.rotation);

        t.world.position += rb.linearVelocity * dt;
        rb.linearVelocity += a * dt;

        glm::quat dq = Gep::Derivative(t.world.rotation, rb.angularVelocity);
        t.world.rotation += dq * dt;
        t.world.rotation = glm::normalize(t.world.rotation);

        rb.angularVelocity += alpha * dt;
    }

    void PhysicsSystem::Integrate_SemiImplicitEuler(RigidBody& rb, Transform& t, float dt) const
    {
        glm::vec3 a = rb.LinearAcceleration();
        glm::vec3 alpha = rb.AngularAcceleration(t.world.rotation);

        rb.linearVelocity += a * dt;
        t.world.position += rb.linearVelocity * dt;

        rb.angularVelocity += alpha * dt;
        glm::quat dq = Gep::Derivative(t.world.rotation, rb.angularVelocity);
        t.world.rotation += dq * dt;
        t.world.rotation = glm::normalize(t.world.rotation);
    }

    void PhysicsSystem::Integrate_Verlet(RigidBody& rb, Transform& t, float dt) const
    {
        const float dt2 = dt * dt;

        // linear term
        const glm::vec3 linearAcceleration = rb.LinearAcceleration();
        t.world.position += rb.linearVelocity * dt + 0.5f * linearAcceleration * dt2;
        rb.linearVelocity += linearAcceleration * dt;

        // angular term
        const glm::vec3 angularAcceleration = rb.AngularAcceleration(t.world.rotation);
        const glm::vec3 omegaHalfStep = rb.angularVelocity + 0.5f * angularAcceleration * dt;

        glm::quat nextRotation = t.world.rotation;
        glm::quat dq = Gep::Derivative(nextRotation, omegaHalfStep);
        nextRotation += dq * dt;
        t.world.rotation = glm::normalize(nextRotation);

        rb.angularVelocity += angularAcceleration * dt;
    }

    constexpr float kMinSpringStiffness = 0.0f;
    constexpr float kMaxSpringStiffness = 1.0e5f; // tweak to taste

    void PhysicsSystem::ApplySpringForces()
    {
        mManager.ForEachArchetype([&](Gep::Entity e, Spring& spring)
        {
            if (spring.stiffness <= 0.0f || !spring.startEntity.is_valid())
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
        ApplySpringForces();

        mManager.ForEachArchetype([&](Gep::Entity e, RigidBody& rb, Transform& t)
        {
            rb.ApplyForce({ 0.0f, -9.81f, 0.0f });

            switch (mCurrentPhysicsIntegration)
            {
                case 0: Integrate_ExplicitEuler    (rb, t, dt); break;
                case 1: Integrate_SemiImplicitEuler(rb, t, dt); break;
                case 2: Integrate_Verlet           (rb, t, dt); break;
                case 3: Integrate_RungeKutta4      (rb, t, dt); break;
            }

            // update the entities local transformation
            Gep::Entity p = mManager.GetParent(e);
            if (mManager.EntityExists(p) && mManager.HasComponent<Transform>(p))
            {
                Transform& pt = mManager.GetComponent<Transform>(p);
                t.local = Gep::Inverse(pt.world) * t.world;
            }
            else
                t.local = t.world;

            rb.ClearAccumulators();
        });
    }

    void PhysicsSystem::DrawSprings()
    {
        const glm::vec3 kRestColor{ 0.0f, 0.0f, 1.0f }; // blue
        const glm::vec3 kMidColor{ 1.0f, 1.0f, 0.0f };  // yellow
        const glm::vec3 kMaxColor{ 1.0f, 0.0f, 0.0f };  // red

        const auto evaluateColor = [&](float normalizedStretch)
        {
            normalizedStretch = std::clamp(normalizedStretch, 0.0f, 1.0f);

            if (normalizedStretch <= 0.5f)
            {
                const float t = normalizedStretch * 2.0f;
                return glm::mix(kRestColor, kMidColor, t);
            }

            const float t = (normalizedStretch - 0.5f) * 2.0f;
            return glm::mix(kMidColor, kMaxColor, t);
        };

        Gep::LineGPUData line;
        line.color = { 0.0f, 0.0f, 1.0f };
        mManager.ForEachArchetype([&](Gep::Entity e, Spring& spring)
        {

            const Gep::Entity startEntity = mManager.FindEntity(spring.startEntity);
            const Gep::Entity endEntity = mManager.FindEntity(spring.endEntity);

            if (!mManager.EntityExists(startEntity) || !mManager.EntityExists(endEntity))
                return;

            if (!mManager.HasComponent<Transform>(startEntity) || !mManager.HasComponent<Transform>(endEntity))
                return;

            Transform& startTransform = mManager.GetComponent<Transform>(startEntity);
            Transform& endTransform = mManager.GetComponent<Transform>(endEntity);

            const float restLength = std::max(0.0f, spring.restLength);
            const float distance = glm::distance(startTransform.world.position, endTransform.world.position);
            const float stretch = std::max(0.0f, distance - restLength);

            // Stretch needed to reach “red.” If the spring has a non-zero rest length we use 50% of it,
            // otherwise fall back to the current distance to keep zero-rest springs responsive.
            const float stretchForMax = std::max(0.01f, (restLength > 1e-3f ? restLength * 0.5f : distance));

            //line.color = evaluateColor(stretchForMax > 0.0f ? stretch / stretchForMax : 0.0f);
            line.points.push_back({ startTransform.world.position, endTransform.world.position });

        });
        mRenderer.AddLine(line);

    }

    void PhysicsSystem::OnSpringEditorRender(const Gep::Event::ComponentEditorRender<Spring>&event)
    {
        EditorResource& editor = mManager.GetResource<EditorResource>();

        bool validStart = editor.DrawEntityDragDropTarget<Client::Transform>(mManager, "Start Entity", event.components,
            [&](Spring* spring) -> gtl::uuid& { return spring->startEntity; }
        );

        //TODO make this give better error messages/feedback. ex: if transform is missing give a message
        bool validEnd = editor.DrawEntityDragDropTarget<Client::Transform>(mManager, "End Entity", event.components,
            [&](Spring* spring) -> gtl::uuid& { return spring->endEntity; }
        );

        if (ImGui::Button("Match Rest Length"))
        {
            for (Spring* s : event.components)
            {
                Transform& startT = mManager.GetComponent<Transform>(mManager.FindEntity(s->startEntity));
                Transform& endT = mManager.GetComponent<Transform>(mManager.FindEntity(s->endEntity));
                s->restLength = glm::distance(startT.world.position, endT.world.position);
            }
        }

        bool restLengthChanged = Gep::ImGui::MultiDragFloat("Rest Length", event.components,
            [](Spring* s) -> float& { return s->restLength; }
        );

        bool stiffnessChanged = Gep::ImGui::MultiDragFloat("Stiffness", event.components,
            [](Spring* s) -> float& { return s->stiffness; }
        );

        bool dampingChanged = Gep::ImGui::MultiDragFloat("Damping", event.components,
            [](Spring* s) -> float& { return s->damping; }
        );

        if (restLengthChanged)
        {
            for (Spring* s : event.components)
                s->restLength = std::max(0.0f, s->restLength);
        }

        if (stiffnessChanged)
        {
            for (Spring* s : event.components)
                s->stiffness = std::clamp(s->stiffness, kMinSpringStiffness, kMaxSpringStiffness);
        }

        if (dampingChanged)
        {
            for (Spring* s : event.components)
                s->damping = std::max(0.0f, s->damping);
        }
    }

    void PhysicsSystem::OnSpringAdded(const Gep::Event::ComponentAdded<Spring>& event)
    {

    }

    void PhysicsSystem::HandleInputs()
    {
        GLFWwindow* window = glfwGetCurrentContext();

        static bool isF8 = false;
        static bool isF9 = false;

        // for physics rotate
        if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS)
        {
            if (!isF8)
            {
                mCurrentPhysicsIntegration++;
                mCurrentPhysicsIntegration %= 4;
                isF8 = true;

                switch (mCurrentPhysicsIntegration)
                {
                case 0: Gep::Log::Important("Current Physics Integration Technique: ExplicitEuler"); break;
                case 1: Gep::Log::Important("Current Physics Integration Technique: SemiImplicitEuler"); break;
                case 2: Gep::Log::Important("Current Physics Integration Technique: Verlet"); break;
                case 3: Gep::Log::Important("Current Physics Integration Technique: RungeKutta4"); break;
                }

            }
        }
        else
            isF8 = false;

        // for spring render
        if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS)
        {
            if (!isF9)
            {
                mRenderSprings = !mRenderSprings;
                isF9 = true;
            }
        }
        else
            isF9 = false;
    }


}

