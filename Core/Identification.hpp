/*****************************************************************//**
 * \file   Identification.hpp
 * \brief  Stores information to identify entities, such as a name
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

#include <sol/sol.hpp>

namespace Client
{
    struct Identification
    {
        std::string name = "";

        void OnScriptAccess(sol::table& table)
        {
            table["name"] = name;
        }
    };
}
