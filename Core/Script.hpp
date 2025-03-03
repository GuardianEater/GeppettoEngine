/*****************************************************************//**
 * \file   Script.hpp
 * \brief  the component that contains script data for an entity
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <string>
#include "ScriptingResource.hpp"
#include "EngineManager.hpp"

namespace Client
{
    struct Script
    {
        std::filesystem::path path = "assets\\scripts\\example.lua";
        sol::environment env;

        sol::function init;
        sol::function update;
        sol::function exit;

        void OnImGuiRender(Gep::EngineManager& em)
        {
            ImGui::Text("Script Path: %s", path.string().c_str());
            ScriptingResource& sr = em.GetResource<ScriptingResource>();
            sol::state& lua = sr.GetLua();
            const std::set<std::filesystem::path>& knownScripts = sr.GetKnownScripts();

            if (ImGui::Button("Locate new scripts", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                sr.LocateScripts();
            }

            if (ImGui::Button("Reload script", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
            {
                LoadScript(lua, path);
            }

            // drop down for selecting a script

            bool scriptsOpen = ImGui::BeginCombo("Scripts", path.filename().string().c_str());

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(char) * (strlen((const char*)payload->Data) + 1));
                    const char* path = (const char*)payload->Data;
                    std::filesystem::path droppedPath(path);

                    LoadScript(lua, droppedPath);
                }
                ImGui::EndDragDropTarget();
            }

            if (scriptsOpen)
            {
                for (const auto& script : knownScripts)
                {
                    bool isSelected = path == script;
                    if (ImGui::Selectable(script.filename().string().c_str(), isSelected))
                    {
                        LoadScript(lua, script);
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
                for (auto& [key, value] : env)
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
                        {
                            ImGui::Text("function");
                            sol::function func = value;
                            sol::table info = env["debug"]["getinfo"](func);
                            int numParams = info["nparams"];
                            ImGui::SameLine();
                            ImGui::Text("params: %d", numParams);
                            break;
                        }
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
        }

        void LoadScript(sol::state& lua, const std::filesystem::path& newPath)
        {
            if (!std::filesystem::exists(newPath))
            {
                Gep::Log::Error("Failed to load script the given path doesn't exist: [", newPath.string(), "]");
                return;
            }

            path = newPath;

            env = sol::environment(lua, sol::create, lua.globals());
            sol::protected_function_result result = lua.safe_script_file(path.string(), env, sol::script_pass_on_error);

            if (!result.valid())
            {
                sol::error err = result;
                Gep::Log::Error("Error loading script: ", err.what());
                return;
            }

            init   = env["Initialize"];
            update = env["Update"];
            exit   = env["Exit"];
        }
    };
}
