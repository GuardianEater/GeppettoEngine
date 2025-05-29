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
    }

    void ScriptingSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Script>>(this, &ScriptingSystem::OnScriptAdded);
        mManager.SubscribeToEvent<Gep::Event::EntityCreated>(this, &ScriptingSystem::OnEntityCreated);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Script>>(this, &ScriptingSystem::OnScriptEditorRender);

        mManager.GetResource<ScriptingResource>().LocateScripts();
    }

    void ScriptingSystem::Update(float dt)
    {
        sol::state& lua = mManager.GetResource<ScriptingResource>().GetLua();

        mManager.ForEachArchetype<Script>([&](Gep::Entity entity, Script& script)
        {
            sol::table self = lua.create_table();

            mManager.ForEachComponent(entity, [&](const Gep::ComponentData& data)
            {
                mSetComponentMemberReferences[data.index](entity, self);
            });

            if (!script.env.valid())
            {
                // this will always happen once because component construction is not yet deffered
                Gep::Log::Error("ScriptingSystem::Update() failed, script environment is invalid on entity: [", entity, "]");

                return; // note: this is the same as continue in the foreach loop
            }

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
        });
    }

    void ScriptingSystem::OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event)
    {
        Script& script = mManager.GetComponent<Client::Script>(event.entity);
        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();
        
        script.LoadScript(sr.GetLua(), script.path);
    }

    void ScriptingSystem::OnScriptEditorRender(const Gep::Event::ComponentEditorRender<Script>& event)
    {
        Script& script = event.component;

        if (!script.env)
        {
            ImGui::TextColored(ImVec4{ 1,0,0,1 }, "Enviroment is invalid!");
            return;
        }

        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();
        EditorResource& er = mManager.GetResource<EditorResource>();
        sol::state& lua = sr.GetLua();
        const std::set<std::filesystem::path>& knownScripts = sr.GetKnownScripts();

        ImGui::Text("Script Path: %s", script.path.string().c_str());
        if (ImGui::Button("Locate new scripts", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            sr.LocateScripts();
        }

        if (ImGui::Button("Reload script", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            script.LoadScript(lua, script.path);
        }

        // drop down for selecting a script

        bool scriptsOpen = ImGui::BeginCombo("Scripts", script.path.filename().string().c_str());

        er.AssetBrowserDropTarget({ ".lua" }, [&](const std::filesystem::path& droppedPath)
            {
                script.LoadScript(lua, droppedPath);
            });

        if (scriptsOpen)
        {
            for (const auto& loadedScript : knownScripts)
            {
                bool isSelected = script.path == loadedScript;
                if (ImGui::Selectable(loadedScript.filename().string().c_str(), isSelected))
                {
                    script.LoadScript(lua, loadedScript);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

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
                    std::string keyString = key.as<std::string>().c_str();
                    ImGui::TableNextRow();

                    // First column: key text
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", keyString.c_str());

                    // Second column: value text based on type
                    ImGui::TableSetColumnIndex(1);
                    switch (value.get_type())
                    {
                    case sol::type::string:
                    {
                        std::string s = value.as<std::string>();
                        if (ImGui::InputText((std::string("##") + keyString).c_str(), &s))
                        {
                            script.env[key] = s;
                        }
                        break;
                    }
                    case sol::type::number:
                    {
                        float f = value.as<float>();
                        if (ImGui::InputFloat((std::string("##") + keyString).c_str(), &f))
                        {
                            script.env[key] = f;
                        }
                        break;
                    }
                    case sol::type::boolean:
                    {
                        bool b = value.as<float>();
                        if (ImGui::Checkbox((std::string("##") + keyString).c_str(), &b))
                        {
                            script.env[key] = b;
                        }
                        break;
                    }
                    case sol::type::table:
                    {
                        ImGui::TextDisabled("Table");
                        break;
                    }
                    case sol::type::function:
                    {
                        // sol::function func = value;
                        // sol::table info = script.env["debug"]["getinfo"](func);
                        // int numParams = info["nparams"];
                        ImGui::TextDisabled("Function");
                        break;
                    }
                    case sol::type::userdata:
                        ImGui::TextDisabled("userdata");
                        break;
                    default:
                        ImGui::TextDisabled("???");
                        break;
                    }
                }
            }
            ImGui::EndTable();
        }
    }

    void ScriptingSystem::OnEntityCreated(const Gep::Event::EntityCreated& event)
    {
    }
}


