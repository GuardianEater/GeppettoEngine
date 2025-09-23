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

        py::module module{};

        py::function on_enabled{};
        py::function on_start{};
        py::function update{};
        //py::function fixedUpdate; maybe useful?
        py::function late_update{};
        py::function on_disable{};
        py::function on_destroy{};

        // on start   -> when the object is created
        // on destroy -> when the object is destroyed

        void LoadScript(const std::filesystem::path& newPath)
        {
            if (!std::filesystem::exists(newPath))
            {
                Gep::Log::Error("Failed to load script the given path doesn't exist: [", newPath.string(), "]");
                return;
            }

            py::module sys = py::module::import("sys");
            auto path = sys.attr("path").cast<py::list>();
            path.append(newPath.parent_path().string().c_str());

            module = py::module::import(newPath.stem().string().c_str());

            // binds init function if it has one
            if (py::hasattr(module, "on_enabled"))
            { 
                on_enabled = module.attr("on_enabled");
            }

            if (py::hasattr(module, "on_start"))
            {
                on_start = module.attr("on_start");
            }

            if (py::hasattr(module, "update"))
            {
                update = module.attr("update");
            }

            if (py::hasattr(module, "late_update"))
            {
                late_update = module.attr("late_update");
            }

            if (py::hasattr(module, "on_disable"))
            {
                late_update = module.attr("on_disable");
            }
            
            if (py::hasattr(module, "on_destroy"))
            {
                late_update = module.attr("on_destroy");
            }
        }
    };
}
