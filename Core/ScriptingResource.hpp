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
        ScriptingResource();

        void LocateScripts();
        sol::state& GetLua();
        const std::set<std::filesystem::path>& GetKnownScripts() const;

    private:
        std::set<std::filesystem::path> mKnownScripts;
        sol::state mLua;
    };
}