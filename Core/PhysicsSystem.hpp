/*****************************************************************//**
 * \file   PhysicsSystem.hpp
 * \brief  test physics simulation
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

// backend
#include "ISystem.hpp"

namespace Gep::Event
{
    struct EntityDestroyed;
    struct KeyPressed;

    template <typename T> struct ComponentAdded;
    template <typename T> struct ComponentEditorRender;
}

namespace Gep
{
    class EngineManager;
    class OpenGLRenderer;
}

namespace Client
{
    struct Transform;
    struct RigidBody;
    struct Spring;
}

namespace Client
{

    class PhysicsSystem : public Gep::ISystem
    {
    public:
        PhysicsSystem(Gep::EngineManager& em);
        ~PhysicsSystem();

    private:
        void Update(float dt) override;
        void FrameEnd() override;

        void OnTransformEditorRender(const Gep::Event::ComponentEditorRender<Transform>& event);
        void OnRigidBodyEditorRender(const Gep::Event::ComponentEditorRender<RigidBody>& event);
        void OnSpringEditorRender(const Gep::Event::ComponentEditorRender<Spring>& event);

        void OnRigidBodyAdded(const Gep::Event::ComponentAdded<RigidBody>& event);
        void OnSpringAdded(const Gep::Event::ComponentAdded<Spring>&event);

        // physics computation

        void Integrate_RungeKutta4(RigidBody& rb, Transform& t, float dt) const;
        void Integrate_ExplicitEuler(RigidBody& rb, Transform& t, float dt) const;
        void Integrate_SemiImplicitEuler(RigidBody& rb, Transform& t,  float dt) const;
        void Integrate_Verlet(RigidBody& rb, Transform& t, float dt) const;

        void ApplySpringForces();
        void Integrate(float dt);
        void DrawSprings();

        void HandleInputs();

    private:
        Gep::OpenGLRenderer& mRenderer;

        bool mRenderSprings = true;
        uint8_t mCurrentPhysicsIntegration = 0;
        const float mFixedTimeStep = 1.0f / 60.0f;
        const float mMaxAccumulatedTime = 0.25f;
        float mStepAccumulator = 0.0f;
    };
}

