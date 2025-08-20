/*****************************************************************//**
 * \file   AssetLocation.hpp
 * \brief  similar to a filepath except doesnt necessarily store the location of something on disk.
 *         may also store the location of something in source, or from the internet.
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   August 2025
 *********************************************************************/

#pragma once

#include <string>

namespace Gep
{
    struct AssetLocation
    {
        enum class Type
        {
            Filesystem,
            Internal, // predefined type defined by the program
            Internet,
        };

        std::string location;
        AssetLocation::Type type;
    };
}
