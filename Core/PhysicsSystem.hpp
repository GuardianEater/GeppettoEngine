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
}

namespace Client
{
    struct Transform;
    struct RigidBody;
    struct RawModelComponent;
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

        void OnRawModelAdded(const Gep::Event::ComponentAdded<RawModelComponent>& event);
        void OnRigidBodyAdded(const Gep::Event::ComponentAdded<RigidBody>& event);
    };
}

