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
    GLuint Texture::UploadToGPU() const
    {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.data());

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    }

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

    const std::vector<std::string>& Texture::SupportedExtensions()
    {
        static std::vector<std::string> extensions = { ".jpg", ".jpeg", ".png", ".bmp" };

        return extensions;
    }
}

