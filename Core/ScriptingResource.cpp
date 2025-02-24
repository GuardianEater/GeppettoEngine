/*****************************************************************//**
 * \file   ScriptingResource.cpp
 * \brief  implementation for the ScriptingResource
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"

#include "ScriptingResource.hpp"

namespace Client
{
    void ScriptingResource::LoadScript(const std::string& name, const std::filesystem::path& scriptPath)
    {
        if (mScripts.contains(name))
        {
            Gep::Log::Error("Cannot load script: [", name, "] that name has already been loaded");
            return;
        }

        if (!std::filesystem::exists(scriptPath))
        {
            Gep::Log::Error("Cannot load script: [", scriptPath.string(), "] the file does not exist");
            return;
        }

        ScriptData data{ 
            .path = scriptPath,
            .script = mLua.load_file(scriptPath.string())
        };

        mScripts.emplace(name, data);

        sol::load_result& script = mScripts.at(name).script;

        if (!script.valid())
        {
            sol::error err = script;
            Gep::Log::Error("Failed to load script: [", scriptPath.string(), "] ", err.what());
            return;
        }

        Gep::Log::Info("Loaded Script: [", name, "] at [", scriptPath.string(), "]");
    }
    void ScriptingResource::UnloadScript(const std::string& name)
    {
        if (!mScripts.contains(name))
        {
            Gep::Log::Error("Cannot unload script: [", name, "] that name has not been loaded");
            return;
        }

        mScripts.erase(name);

        Gep::Log::Info("Unloaded Script: [", name, "]");
    }

    void ScriptingResource::ReloadScript(const std::string& name)
    {
        if (!mScripts.contains(name))
        {
            Gep::Log::Error("Cannot reload script: [", name, "] that name has not been loaded");
            return;
        }

        std::filesystem::path path = mScripts.at(name).path;
        UnloadScript(name);
        LoadScript(name, path);
    }
}
