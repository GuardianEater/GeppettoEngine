/*****************************************************************//**
 * \file   ModelSerializer.hpp
 * \brief  
 * 
 * \author 2018t
 * \date   April 2026
 *********************************************************************/

#pragma once

// std
#include <vector>
#include <gtl/binary_buffer.hpp>

// local
#include "Model.hpp"
#include <filesystem>
#include "assimp/scene.h"
#include "assimp/material.h"

namespace Gep
{
    // turns a model into a string of bytes
    gtl::binary_buffer SerializeModel(const Model& model);

    // turns a string of bytes into a model
    Model DeserializeModel(const gtl::binary_buffer& bytes);

    // takes external filepaths and converts them into engine data
    template <typename AssetType>
    struct ImportedAsset
    {
        gtl::uuid uuid;
        AssetType asset;
    };

    // imports from common formats into an internal engine format
    class Importer
    {
    public:
        void Import(const std::filesystem::path& path);
        // animations
        // scenes
        // ...
    private:

        struct MaterialAsset
        {
            float ao = 1.0f; // ambient occlusion
            float roughness = 0.8f;
            float metalness = 0.0f;
            float emission = 0.0f;
            glm::vec4 color = { 0.8f, 0.8f, 0.8f, 1.0f };

            gtl::uuid aoTexture{};
            gtl::uuid roughnessTexture{};
            gtl::uuid metalnessTexture{};
            gtl::uuid diffuseTexture{};
            gtl::uuid normalTexture{};
            gtl::uuid emissionTexture{};
        };

        struct TextureAsset
        {
            std::vector<std::byte> bytes;
            glm::uvec2 size;

            static TextureAsset FromMemory(const uint8_t* bytes, size_t size);
            static TextureAsset FromPixels(const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);
            static TextureAsset FromFile(const std::filesystem::path& path);
        };

    private:

        gtl::uuid ExtractTexture(const std::filesystem::path& path, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type);
        gtl::uuid ExtractMaterial(const std::filesystem::path& path, const aiScene* scene, const aiMaterial* assimpMaterial);

    private:

        std::unordered_map<std::string, gtl::uuid> mPathToUUID;

        std::unordered_map<gtl::uuid, Gep::Model>     mModels;
        std::unordered_map<gtl::uuid, TextureAsset>   mTextures;
        std::unordered_map<gtl::uuid, Gep::Animation> mAnimations;
        std::unordered_map<gtl::uuid, MaterialAsset>  mMaterials;
    };
}
