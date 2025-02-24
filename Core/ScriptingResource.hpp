/*****************************************************************//**
 * \file   ScriptingResource.hpp
 * \brief  holds onto compiled scripts
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include <sol/sol.hpp>

namespace Client
{
    class ScriptingResource
    {
    public:
        void LoadScript(const std::string& name, const std::filesystem::path& scriptPath);
        void UnloadScript(const std::string& name);
        void ReloadScript(const std::string& name);

        struct ScriptData
        {
            std::filesystem::path path;
            sol::load_result script;
        };


        std::unordered_map<std::string, ScriptData> mScripts;
        sol::state mLua;
    };
}