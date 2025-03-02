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
    ScriptingResource::ScriptingResource()
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

    void ScriptingResource::LocateScripts()
    {
        mKnownScripts.clear();

        // get all files in the scripts directory
        for (const auto& entry : std::filesystem::directory_iterator("assets\\scripts"))
        {
            mKnownScripts.insert(entry.path());
        }
    }
    sol::state& ScriptingResource::GetLua()
    {
        return mLua;
    }
    const std::set<std::filesystem::path>& ScriptingResource::GetKnownScripts() const
    {
        return mKnownScripts;
    }
}
