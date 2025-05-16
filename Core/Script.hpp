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
#include "EditorResource.hpp"
#include "EngineManager.hpp"

namespace Client
{
    struct Script
    {
        std::filesystem::path path = "assets\\scripts\\example.lua";
        sol::environment env;

        sol::protected_function init;
        sol::protected_function update;
        sol::protected_function exit;

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
