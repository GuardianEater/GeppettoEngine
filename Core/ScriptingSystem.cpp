/*****************************************************************//**
 * \file   ScriptingSystem.cpp
 * \brief  allows adding scripts to entities
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "pch.hpp"

#include "ScriptingSystem.hpp"
#include <Transform.hpp>
#include <RigidBody.hpp>
#include <sol/sol.hpp>

namespace Client
{
    static sol::state lua;

    ScriptingSystem::ScriptingSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    void ScriptingSystem::Initialize()
    {
        lua.open_libraries();

        lua.new_usertype<glm::vec3>("Vec3",
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z
        );

        lua.new_usertype<Client::Transform>("Transform",
            "position", &Transform::position,
            "scale", &Transform::scale,
            "rotation", &Transform::rotation
        );

        lua.new_usertype<Client::RigidBody>("RigidBody",
            "velocity", &RigidBody::velocity,
            "acceleration", &RigidBody::acceleration,
            "rotational", &RigidBody::rotationalVelocity
        );
    }

    void ScriptingSystem::Update(float dt)
    {
        std::vector<Gep::Entity>& entities = mManager.GetEntities<Script>();
        for (Gep::Entity entity : entities)
        {
            Script& script = mManager.GetComponent<Client::Script>(entity);

            if (mManager.HasComponent<Transform>(entity))
            {
                lua["Transform"] = &mManager.GetComponent<Transform>(entity);
            }
            if (mManager.HasComponent<RigidBody>(entity))
            {
                lua["RigidBody"] = &mManager.GetComponent<RigidBody>(entity);
            }

            lua.script(script.data, sol::script_pass_on_error);
        }
    }
}


