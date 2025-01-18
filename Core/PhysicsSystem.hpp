/*****************************************************************//**
 * \file   PhysicsSystem.hpp
 * \brief  test physics simulation
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

 // core
#include <Core.hpp>
#include <glm.hpp>
#include <gtc/quaternion.hpp>

// backend
#include <ISystem.hpp>
#include <EngineManager.hpp>
#include <Affine.hpp>

// client
#include <Transform.hpp>
#include <RigidBody.hpp>

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

