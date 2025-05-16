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
#include "Events.hpp"

namespace Gep
{
    namespace Event
    {
        struct EntityDestroyed;
        struct KeyPressed;
    }

    class EngineManager;
}

namespace Client
{
    struct Transform;
    struct RigidBody;

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
    };
}

