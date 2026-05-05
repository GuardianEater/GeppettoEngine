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
#include <gtl/uuid.hpp>
#include <cstddef>

namespace Gep
{
    struct Texture
    {
        gtl::uuid uuid;

        GLuint64 handle = NULL; // gpu side pointer
        GLuint id       = NULL; // bindable id
        glm::uvec2 size{};
        GLint format{};

        std::vector<std::byte> GetPixels(GLenum format, GLenum type, uint32_t channels) const;

        static Texture LoadFileIcon(const std::filesystem::path& path);

        static Texture Load(const std::filesystem::path& path);
        static Texture LoadFromPixels(const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);
        static Texture LoadFromMemory(const uint8_t* imageFileData, size_t size);
        static Texture LoadFromUUID(const gtl::uuid& uuid);

        static Texture LoadHDR(const std::filesystem::path& path);
        static Texture LoadFromPixelsHDR(const float* pixelData, size_t width, size_t height, int requiredChannels);

        static Texture Gen2D(glm::uvec2 size, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT, GLint wrapParam = GL_CLAMP_TO_EDGE);

    };

    // contains material data for pbr rendering.
    struct Material
    {
        gtl::uuid uuid;

        // if it has a uniform material across the entire mesh
        float ao = 1.0f; // ambient occlusion
        float roughness = 0.8f;
        float metalness = 0.0f;
        float emission = 0.0f;
        glm::vec4 color = { 0.8f, 0.8f, 0.8f, 1.0f };

        Texture aoTexture{};
        Texture roughnessTexture{};
        Texture metalnessTexture{};
        Texture diffuseTexture{};
        Texture normalTexture{};
        Texture emissionTexture{};
    };
}
