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
    ScriptingSystem::ScriptingSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();

        sr.mLua.open_libraries();

        // TODO: need to make this readonly on the lua side
        const sol::table log = sr.mLua.create_table("Log");

        sr.mLua["Log"]["Trace"] = [](const sol::variadic_args& args)
        {
            std::string message;
            for (auto arg : args)
                message += arg.get<std::string>();

            Gep::Log::Trace(message);
        };

        sr.mLua["Log"]["Info"] = [](const sol::variadic_args& args)
        {
            std::string message;
            for (auto arg : args)
                message += arg.get<std::string>();
            Gep::Log::Info(message);
        };

        sr.mLua["Log"]["Warning"] = [](const sol::variadic_args& args)
        {
            std::string message;
            for (auto arg : args)
                message += arg.get<std::string>();
            Gep::Log::Warning(message);
        };

        sr.mLua["Log"]["Error"] = [](const sol::variadic_args& args)
        {
            std::stringstream ss;
            for (auto&& arg : args) {
                if (arg.is<std::string>()) ss << arg.as<std::string>();
                else if (arg.is<int>())    ss << arg.as<int>();
                else if (arg.is<float>())  ss << arg.as<float>();
                else if (arg.is<double>()) ss << arg.as<double>();
                else if (arg.is<bool>())   ss << arg.as<bool>();
                else if (arg.is<glm::vec3>()) {
                    glm::vec3 vec = arg.as<glm::vec3>();
                    ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
                }
                else {
                    ss << "???";
                }
            }

            Gep::Log::Error(ss.str());
        };

        sr.mLua.new_usertype<glm::vec3>("vec3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),

            // Fields
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,

            // Operators
            sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
            sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
            sol::meta_function::multiplication, [](const glm::vec3& a, float scalar) { return a * scalar; },
            sol::meta_function::division, [](const glm::vec3& a, float scalar) { return a / scalar; }
        );
    }

    void ScriptingSystem::Initialize()
    {
        //lua.new_usertype<Client::Transform>("Transform",
        //    "position", &Transform::position,
        //    "scale", &Transform::scale,
        //    "rotation", &Transform::rotation
        //);

        //lua.new_usertype<Client::RigidBody>("RigidBody",
        //    "velocity", &RigidBody::velocity,
        //    "acceleration", &RigidBody::acceleration,
        //    "rotational", &RigidBody::rotationalVelocity
        //);
    }

    void ScriptingSystem::Update(float dt)
    {
        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();

        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Script>();
        for (Gep::Entity entity : entities)
        {
            sol::table entityTable = sr.mLua.create_table();
            sol::environment entityEnvironment(sr.mLua, sol::create, sr.mLua.globals());

            Script& script = mManager.GetComponent<Client::Script>(entity);

            mManager.ForEachComponent(entity, [&](const Gep::ComponentData& data)
                {
                    mSetComponentMemberReferences[data.index](entity, entityTable);
                });

            entityEnvironment["self"] = entityTable;

            static std::string lastError;
            sol::protected_function_result result = sr.mLua.script(script.data, entityEnvironment, sol::script_pass_on_error);

            // prints only the lastest error
            if (!result.valid())
            {
                sol::error err = result;

                if (err.what() != lastError)
                {
                    lastError = err.what();
                    Gep::Log::Error("Error in script on entity [", entity, "]: ", err.what());
                }
            }
        }
    }
}


