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

        py::module module;
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

            //std::string out = (newPath.parent_path() / newPath.stem()).string();

            //Gep::Log::Important(out);

            py::module sys = py::module::import("sys");
            auto path = sys.attr("path").cast<py::list>();
            path.append(newPath.parent_path().string().c_str());

            module = py::module::import(newPath.stem().string().c_str());

            // binds init function if it has one
            if (py::hasattr(module, "init"))
            { 
                init = module.attr("init");
                init();
            }
            else
                init = py::function();

            // binds update function if it has one
            if (py::hasattr(module, "update"))
                update = module.attr("update");
            else
                update = py::function();

            // binds exit function if it has one
            if (py::hasattr(module, "exit"))
                exit = module.attr("exit");
            else
                exit = py::function();
        }
    };
}
