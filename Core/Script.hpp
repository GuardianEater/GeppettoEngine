/*****************************************************************//**
 * \file   Script.hpp
 * \brief  the component that contains script data for an entity
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include "ScriptingResource.hpp"

namespace Client
{
    struct Script
    {
        std::filesystem::path path = "assets\\scripts\\example.py";

        py::function on_enabled{};
        py::function on_start{};
        py::function update{};
        //py::function fixedUpdate; maybe useful?
        py::function late_update{};
        py::function on_disable{};
        py::function on_destroy{};
    };
}
