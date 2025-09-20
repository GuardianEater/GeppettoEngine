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
    class ScriptingResource
    {
    public:
        ScriptingResource();

        void LocateScripts();
        const std::set<std::filesystem::path>& GetKnownScripts() const;

    private:
        std::set<std::filesystem::path> mKnownScripts;
        py::scoped_interpreter mGuard; // starts the python interpretter and closes it when the resource dies
        py::module_ mMain = py::module_::import("__main__");
        py::object mGlobals = mMain.attr("__dict__");
    };
}