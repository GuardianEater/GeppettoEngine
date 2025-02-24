/*****************************************************************//**
 * \file   Script.hpp
 * \brief  the component that contains script data for an entity
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <string>

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
        }
    };
}
