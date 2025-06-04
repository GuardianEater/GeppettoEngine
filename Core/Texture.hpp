/*****************************************************************//**
 * \file   Texture.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <vector>
#include <filesystem>

namespace Gep
{
    struct Texture
    {
        static Texture FromFile(const std::filesystem::path& path);
        static const std::vector<std::string>& SupportedExtensions();

        std::vector<uint8_t> data;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
    };
}
