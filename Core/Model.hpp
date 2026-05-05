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
#include <gtl/uuid.hpp>

namespace Gep
{
    struct Model
    {
        gtl::uuid uuid;
        std::string name;
        std::vector<Mesh> meshes;
    };
}
