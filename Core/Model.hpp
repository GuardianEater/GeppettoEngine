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
        // contructs a model struct with data in a file, will not load images
        static Model FromFile(const std::filesystem::path& path);
        static const std::vector<std::string>& SupportedExtensions();

        std::string name;

        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        Skeleton skeleton;
        Animation animation;
    };
}
