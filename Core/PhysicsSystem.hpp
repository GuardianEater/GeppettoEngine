/*****************************************************************//**
 * \file   PhysicsSystem.hpp
 * \brief  test physics simulation
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

// backend
#include <ISystem.hpp>

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
    class PhysicsSystem : public Gep::ISystem
    {
    public:
        PhysicsSystem(Gep::EngineManager& em);
        ~PhysicsSystem();

        void Update(float dt);
        void EntityDestroyed(const Gep::Event::EntityDestroyed& eventData);
        void KeyPressed(const Gep::Event::KeyPressed& eventData);
    };
}

