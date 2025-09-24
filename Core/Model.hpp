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
    struct ModelNode
    {
        // the indices into the hierarchy on a model to this nodes children. only supports uint16_t max nodes
        std::vector<uint16_t> childrenIndices;
        VQS transformation;
        uint16_t parentIndex;
    };

    struct Model
    {
        // contructs a model struct with data in a file, will not load images
        static Model FromFile(const std::filesystem::path& path);
        static const std::vector<std::string>& SupportedExtensions();

        std::string name;

        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<ModelNode> hierarchy; // flat tree. the first item is always the root. iterate each nodes children for tree descent.
        Skeleton skeleton;
        Animation animation;
    };
}
