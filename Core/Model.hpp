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
        static Model FromFile(const std::filesystem::path& path);
        static const std::vector<std::string>& SupportedExtensions();

        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        std::vector<std::pair<size_t, size_t>> meshMaterialMappings;
    };
}
