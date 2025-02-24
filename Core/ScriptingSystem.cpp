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
        mLua.open_libraries();
        mLua.new_usertype<glm::vec3>("vec3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z
        );

        // TODO: need to make this readonly on the lua side
        const sol::table log = mLua.create_table("Log");

        mLua["Log"]["Trace"] = [](const sol::variadic_args& args)
        {
            std::string message;
            for (auto arg : args)
                message += arg.get<std::string>();

            Gep::Log::Trace(message);
        };

        mLua["Log"]["Info"] = [](const sol::variadic_args& args)
        {
            std::string message;
            for (auto arg : args)
                message += arg.get<std::string>();
            Gep::Log::Info(message);
        };

        mLua["Log"]["Warning"] = [](const sol::variadic_args& args)
        {
            std::string message;
            for (auto arg : args)
                message += arg.get<std::string>();
            Gep::Log::Warning(message);
        };

        mLua["Log"]["Error"] = [](const sol::variadic_args& args)
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

    }

    void ScriptingSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Script>>(this, &ScriptingSystem::OnScriptAdded);
    }

    void ScriptingSystem::Update(float dt)
    {
        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Script>();
        for (Gep::Entity entity : entities)
        {
            Script& script = mManager.GetComponent<Client::Script>(entity);
            //if (!script.env.valid())
            //{
            //    Gep::Log::Error("Script environment is invalid");
            //    continue;
            //}

            sol::table self = mLua.create_table();

            mManager.ForEachComponent(entity, [&](const Gep::ComponentData& data)
            {
                mSetComponentMemberReferences[data.index](entity, self);
            });

            mLua["self"] = self;

            if (script.update.valid())
            {

                sol::protected_function_result updateResult = script.update(dt);
                if (!updateResult.valid())
                {
                    sol::error err = updateResult;
                    Gep::Log::Error("Error running script: ", err.what());
                    script.update = sol::nil;
                }
            }
        }
    }

    void ScriptingSystem::OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event)
    {
        Script& script = mManager.GetComponent<Client::Script>(event.entity);
        //script.env = sol::environment(mLua, sol::create, mLua.globals());
        sol::load_result loadResult = mLua.load_file(script.path.string());
        
        if (!loadResult.valid())
        {
            sol::error err = loadResult;
            Gep::Log::Error("Error loading script: ", err.what());
            return;
        }

        sol::protected_function_result functionResult = loadResult();
        if (!functionResult.valid())
        {
            sol::error err = functionResult;
            Gep::Log::Error("Error starting script: ", err.what());
            return;
        }

        script.init = mLua["Initialize"];
        script.update = mLua["Update"];
        script.exit = mLua["Exit"];

        // run component bindings
    }
}


