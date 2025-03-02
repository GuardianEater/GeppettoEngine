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
            for (auto&& arg : args) 
            {
                if (arg.is<std::string>()) ss << arg.as<std::string>();
                else if (arg.is<int>())    ss << arg.as<int>();
                else if (arg.is<float>())  ss << arg.as<float>();
                else if (arg.is<double>()) ss << arg.as<double>();
                else if (arg.is<bool>())   ss << arg.as<bool>();
                else if (arg.is<glm::vec3>()) 
                {
                    glm::vec3 vec = arg.as<glm::vec3>();
                    ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
                }
                else 
                {
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
            sol::table self = mLua.create_table();

            mManager.ForEachComponent(entity, [&](const Gep::ComponentData& data)
            {
                mSetComponentMemberReferences[data.index](entity, self);
            });

            script.env["self"] = self;

            if (script.update.valid())
            {
                sol::protected_function_result updateResult = script.update(dt);

                if (!updateResult.valid())
                {
                    sol::error err = updateResult;
                    Gep::Log::Error("Error running script: ", err.what());
                    script.update = sol::nil; // prevents the crashed script from running further
                    script.exit = sol::nil;
                }
            }

            ImGui::Begin("Scripts");

            ImGui::Text("Script: %s", script.path.string().c_str());
            ImGui::Text("Entity: %d", entity);

            // Begin a table with 2 columns and some basic flags for borders and row backgrounds
            if (ImGui::BeginTable("##scriptEnv", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                // Setup the table columns
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                // Iterate over the script environment
                for (auto& [key, value] : script.env)
                {
                    // Only show entries with string keys
                    if (key.get_type() == sol::type::string)
                    {
                        ImGui::TableNextRow();

                        // First column: key text
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", key.as<std::string>().c_str());

                        // Second column: value text based on type
                        ImGui::TableSetColumnIndex(1);
                        switch (value.get_type())
                        {
                        case sol::type::string:
                            ImGui::Text("%s", value.as<std::string>().c_str());
                            break;
                        case sol::type::number:
                            ImGui::Text("%f", value.as<float>());
                            break;
                        case sol::type::boolean:
                            ImGui::Text("%s", value.as<bool>() ? "true" : "false");
                            break;
                        case sol::type::table:
                            ImGui::Text("table");
                            break;
                        case sol::type::function:
                            ImGui::Text("function");
                            break;
                        case sol::type::userdata:
                            ImGui::Text("userdata");
                            break;
                        default:
                            ImGui::Text("???");
                            break;
                        }
                    }
                }
                ImGui::EndTable();
            }

            ImGui::End();

        }

    }

    void ScriptingSystem::OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event)
    {
        Script& script = mManager.GetComponent<Client::Script>(event.entity);
        script.env = sol::environment(mLua, sol::create, mLua.globals());
        sol::protected_function_result result = mLua.safe_script_file(script.path.string(), script.env);
        
        if (!result.valid())
        {
            sol::error err = result;
            Gep::Log::Error("Error loading script: ", err.what());
            return;
        }

        script.init = script.env["Initialize"];
        script.update = script.env["Update"];
        script.exit = script.env["Exit"];

        if (script.init.valid())
        {
            sol::protected_function_result initResult = script.init();

            if (!initResult.valid())
            {
                sol::error err = initResult;
                Gep::Log::Error("Error initializing script: ", err.what());
                script.init = sol::nil;
                script.update = sol::nil; // prevents the crashed script from running further
                script.exit = sol::nil;
            }
        }
    }
}


