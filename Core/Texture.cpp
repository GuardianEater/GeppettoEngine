/*****************************************************************//**
 * \file   Texture.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#include "pch.hpp"
#include "Texture.hpp"

#include <stb_image.h>

namespace Gep
{
    Texture Texture::FromFile(const std::filesystem::path& path)
    {
        Texture result{};

        int width;
        int height;
        int channels; // this will be the original value before the image is forced to the required channels
        int requiredChannels = 4; // Force RGBA

        unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, requiredChannels);
        if (!data)
        {
            Gep::Log::Error("Failed to load texture: [", path.string(), "]");
            return {};
        }

        // calculate data total size
        size_t dataSize = static_cast<size_t>(result.width) * result.height * result.channels;

        // move the data into the vector
        result.data.assign(data, data + dataSize);
        result.width = width;
        result.height = height;
        result.channels = requiredChannels; // make sure to use required channels here and not the returned channels

        stbi_image_free(data);

        return result;
    }

    const std::vector<std::string>& SupportedExtensions()
    {
        static std::vector<std::string> extensions = { ".jpg", ".jpeg", ".png", ".bmp" };

        return extensions;
    }
}

