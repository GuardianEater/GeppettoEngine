/*****************************************************************//**
 * \file   Material.hpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#pragma once

#include <glm/glm.hpp>
#include <filesystem>

namespace Gep
{
    struct Texture
    {
        GLuint64 handle = NULL; // gpu side pointer
        GLuint id       = NULL; // bindable id

        static Texture LoadFileIcon(const std::filesystem::path& path);

        static Texture Load(const std::filesystem::path& path);
        static Texture LoadFromPixels(const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);
        static Texture LoadFromMemory(const uint8_t* imageFileData, size_t size);

        static Texture LoadHDR(const std::filesystem::path& path);
        static Texture LoadFromPixelsHDR(const float* pixelData, size_t width, size_t height, int requiredChannels);
    };

    // contains material data for pbr rendering.
    struct Material
    {
        // if it has a uniform material across the entire mesh
        float ao = 1.0f; // ambient occlusion
        float roughness = 0.8f;
        float metalness = 0.8f;
        glm::vec4 color = {1.0f, 0.2f, 0.2f, 1.0f};

        Texture aoTexture{};
        Texture roughnessTexture{};
        Texture metalnessTexture{};
        Texture diffuseTexture{};
        Texture normalTexture{};
    };
}
