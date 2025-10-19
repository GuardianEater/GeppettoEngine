/*****************************************************************//**
 * \file   Model.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include "Mesh.hpp"
#include "Material.hpp"

#include <filesystem>

namespace Gep
{
    struct Model
    {
        std::string name;

        std::vector<Mesh> meshes;
        Skeleton skeleton;
        Animation animation;
    };
}
