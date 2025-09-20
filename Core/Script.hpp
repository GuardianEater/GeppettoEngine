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
        std::filesystem::path path = "assets\\scripts\\example.py";

        py::object module;
        py::function init;
        py::function update;
        py::function exit;

        void LoadScript(const std::filesystem::path& newPath)
        {
            if (!std::filesystem::exists(newPath))
            {
                Gep::Log::Error("Failed to load script the given path doesn't exist: [", newPath.string(), "]");
                return;
            }
        }
    };
}
