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
    // struct that contains all data of a texture
    struct RawTexture
    {
        // takes the stored data and copies it to the gpu,
        // returns the gpu handle
        GLuint UploadToGPU() const; 
        static RawTexture FromFile(const std::filesystem::path& path);
        static const std::vector<std::string>& SupportedExtensions();

        std::vector<uint8_t> data;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
    };

}
