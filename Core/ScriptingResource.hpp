/*****************************************************************//**
 * \file   ScriptingResource.hpp
 * \brief  holds onto compiled scripts
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

namespace Client
{
    struct Script;

    class ScriptingResource
    {
    public:
        ScriptingResource();

        void LocateScripts();
        void ReloadModule(py::module& oldModule);

        // note this will invalidate all scripts that are holding onto a module
        void ReloadAllModules();

        // simply clears all modules so they can be reloaded
        void UnloadAllModules();

        // simply calls a py::function with error checking, will set it to none if it fails
        void PyCall(py::function& func) const;

        // must be called after the scripts module is reloaded
        void BindScriptToModule(Script& script, py::module module) const;

        py::module GetModule(const std::string& name);
        py::module GetOrLoadModule(const std::filesystem::path& path);

        const std::set<std::filesystem::path>& GetKnownScripts() const;

    private:
        std::set<std::filesystem::path> mKnownScripts;
        py::scoped_interpreter mGuard; // starts the python interpretter and closes it when the resource dies
        py::module_ mMain = py::module_::import("__main__");
        py::object mGlobals = mMain.attr("__dict__");

        std::map<std::string, py::module> mModules; // all of the loaded scripts
    };
}