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
#include "Mesh.hpp"
#include <cstddef>

namespace Gep
{
    static const std::string INTERNAL_ASSET_DIR = "internal_assets";

    bool IsAssetImported(const std::string& folder, const gtl::uuid& uuid);

    // turns a model into a string of bytes
    gtl::binary_buffer SerializeModel(const Gep::Model& model);
    gtl::binary_buffer SerializeSkeleton(const Gep::Skeleton& skeleton);
    gtl::binary_buffer SerializeMaterial(const Gep::Material& material);
    gtl::binary_buffer SerializeTexture(const Gep::Texture& texture);
    gtl::binary_buffer SerializeAnimation(const Gep::Animation& animation);

    void SerializeModelToFile(const Gep::Model& model);
    void SerializeSkeletonToFile(const Gep::Skeleton& skeleton);
    void SerializeMaterialToFile(const Gep::Material& material);
    void SerializeTextureToFile(const Gep::Texture& texture);
    void SerializeAnimationToFile(const Gep::Animation& animation);

    // turns a string of bytes into a model
    Gep::Model DeserializeModel(const gtl::binary_buffer& bytes);
    Gep::Skeleton DeserializeSkeleton(const gtl::binary_buffer& bytes);
    Gep::Material DeserializeMaterial(const gtl::binary_buffer& bytes);
    Gep::Texture DeserializeTexture(const gtl::binary_buffer& bytes);
    Gep::Animation DeserializeAnimation(const gtl::binary_buffer& bytes);

    Gep::Model DeserializeModelFromFile(const gtl::uuid& uuid);
    Gep::Skeleton DeserializeSkeletonFromFile(const gtl::uuid& uuid);
    Gep::Material DeserializeMaterialFromFile(const gtl::uuid& uuid);
    Gep::Texture DeserializeTextureFromFile(const gtl::uuid& uuid);
    Gep::Animation DeserializeAnimationFromFile(const gtl::uuid& uuid);

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

        struct TextureAsset
        {
            std::vector<std::byte> bytes;
            glm::uvec2 size{};

            static TextureAsset FromMemory(const uint8_t* bytes, size_t size);
            static TextureAsset FromPixels(const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);
            static TextureAsset FromFile(const std::filesystem::path& path);
        };

        struct BoneInfo
        {
            uint32_t index = 0;
            Gep::VQS offset{};
        };

    private:

        Texture ExtractTexture  (const std::filesystem::path& path, const aiScene* scene, const aiMaterial* assimpMaterial, const aiTextureType type);
        gtl::uuid ExtractMaterial (const std::filesystem::path& path, const aiScene* scene, uint32_t assimpMaterialIdx);
        gtl::uuid ExtractAnimation(const std::filesystem::path& path, const aiScene* scene, uint32_t assimpAnimIdx);
        gtl::uuid ExtractSkeleton (const std::filesystem::path& path, const aiScene* scene);
        gtl::uuid ExtractModel    (const std::filesystem::path& path, const aiScene* scene);

        void ExtractBoneInfo(const aiScene* scene);

        // returns the index of the node just created
        uint32_t FillHierarchyStep(Gep::Skeleton& skeleton, const uint32_t parentIndex, const aiNode* node);

        void FillVertices(Gep::Mesh& mesh, const aiMesh* aMesh);
        void FillIndices(Gep::Mesh& mesh, const aiMesh* aMesh);

        void SetVertexBoneData(Vertex& vertex, uint32_t boneID, float weight);

        void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* assimpMesh, const aiScene* scene);


    private:

        std::unordered_map<std::string, gtl::uuid> mPathToUUID;

        std::unordered_map<gtl::uuid, Gep::Model>     mModels;
        std::unordered_map<gtl::uuid, Gep::Texture>   mTextures;
        std::unordered_map<gtl::uuid, Gep::Animation> mAnimations;
        std::unordered_map<gtl::uuid, Gep::Material>  mMaterials;
        std::unordered_map<gtl::uuid, Gep::Skeleton>  mSkeletons;

        std::unordered_map<std::string, BoneInfo> mBoneInfos;
    };
}
