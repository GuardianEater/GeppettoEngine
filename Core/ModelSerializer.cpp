/*****************************************************************//**
 * \file   ModelSerializer.cpp
 * \brief  
 * 
 * \author 2018t
 * \date   April 2026
 *********************************************************************/

#include "pch.hpp"

#include "ModelSerializer.hpp"

#include <gtl/binary_buffer.hpp>

namespace Gep
{
    gtl::binary_buffer Gep::SerializeModel(const Model& model)
    {
        gtl::binary_buffer bin;

        bin.add(model.name);

        // write meshes
        bin.add(model.meshes.size());
        for (const Mesh& mesh : model.meshes)
        {
            bin.add(mesh.name);
            bin.add(mesh.materialUUID); // need to get this from the material idx?
            bin.add(mesh.boundingBox);
            bin.add(mesh.vertices);
            bin.add(mesh.boneIndices);
            bin.add(mesh.indices);
        }

        // write bones
        bin.add(model.skeleton.bones.size());
        for (const Bone& bone : model.skeleton.bones)
        {
            bin.add(bone.childrenIndices);
            bin.add(bone.inverseBind);
            bin.add(bone.name);
            bin.add(bone.parentIndex);
            bin.add(bone.transformation);
        }

        return bin;
    }

    Model Gep::DeserializeModel(const gtl::binary_buffer& bin)
    {
        Model model;

        bin.get(model.name);

        // read all meshes
        size_t meshesSize = 0;
        bin.get(meshesSize);
        for (size_t i = 0; i < meshesSize; ++i)
        {
            Mesh& mesh = model.meshes.emplace_back();
            bin.get(mesh.name);
            bin.get(mesh.materialUUID);
            bin.get(mesh.boundingBox);
            bin.get(mesh.vertices);
            bin.get(mesh.boneIndices);
            bin.get(mesh.indices);
        }

        // read all bones
        size_t bonesSize = 0;
        bin.get(bonesSize);
        for (size_t i = 0; i < bonesSize; i++)
        {
            Bone& bone = model.skeleton.bones.emplace_back();
            bin.get(bone.childrenIndices);
            bin.get(bone.inverseBind);
            bin.get(bone.name);
            bin.get(bone.parentIndex);
            bin.get(bone.transformation);
        }

        return model;
    }

    void Importer::Import(const std::filesystem::path& path)
    {
        gBoneData.clear();
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType |
            aiProcess_OptimizeGraph |
            aiProcess_OptimizeMeshes
        );

        if (!scene || !scene->mRootNode)
        {
            Gep::Log::Error("Assimp error: ", importer.GetErrorString());
            return;
        }

        Gep::Model model;

        model.name = path.string();

        // loads all of the materials out of this scene
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            const std::string matName = std::to_string(i);
            if (mPathToUUID.contains(matName))
                continue;

            auto matUUID = ExtractMaterial(path, scene, scene->mMaterials[i]);
            mPathToUUID[matName] = matUUID;
        }

        // loads every bone name to its offset matrix in gBoneData
        LoadBoneData(scene);

        // fills in the skeleton of the model and the index field in gBoneData
        LoadHierarchy(model, scene);

        LoadMeshes(model, scene); //Broken?

        LoadAnimations(path.string(), model, scene);

        return model;
    }

    gtl::uuid Importer::ExtractTexture(const std::filesystem::path& path, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type)
    {
        aiString assimpTexturePath;
        if (aiReturn_SUCCESS != assimpMaterial->GetTexture(type, 0, &assimpTexturePath))
            return {}; // this material does not contain a texture of the given type

        const std::string texturePath = assimpTexturePath.C_Str();

        const auto it = mPathToUUID.find(texturePath);
        if (it != mPathToUUID.end())
            return it->second;

        gtl::uuid textureUUID = gtl::generate_uuid();
        mPathToUUID[texturePath] = textureUUID;

        if (texturePath[0] == '*') // if the first character is a star it is embedded
        {
            int assimpTextureIndex = std::atoi(&texturePath[1]);
            aiTexture* assimpTexture = scene->mTextures[assimpTextureIndex];

            if (assimpTexture->mHeight == 0) // if no height then it is compressed
            {
                std::vector<uint8_t> bytes(
                    reinterpret_cast<uint8_t*>(assimpTexture->pcData),
                    reinterpret_cast<uint8_t*>(assimpTexture->pcData) + assimpTexture->mWidth
                );

                mTextures[textureUUID] = TextureAsset::FromMemory(bytes.data(), bytes.size());
            }
            else
            {
                const int assimpTextureChannels = 4;
                mTextures[textureUUID] = TextureAsset::FromPixels(
                    reinterpret_cast<uint8_t*>(assimpTexture->pcData), 
                    assimpTexture->mWidth, 
                    assimpTexture->mHeight, 
                    assimpTextureChannels
                );
            }
        }
        else
        {
            mTextures[textureUUID] = TextureAsset::FromFile(path.parent_path() / texturePath);
        }

        return textureUUID;
    }

    // cleared once per call to load materials. used to map assimp material indexes to the internal mMaterials indexes
    gtl::uuid Importer::ExtractMaterial(const std::filesystem::path& path, const aiScene* scene, const aiMaterial* assimpMaterial)
    {
        MaterialAsset material;
        gtl::uuid materialUUID = gtl::generate_uuid();

        aiColor3D outColor(1.f, 1.f, 1.f);
        if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, outColor))
            material.color = { outColor.r, outColor.g, outColor.b, 1.0f };
        //if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, outColor))
        //    material.ao = outColor.r;
        if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, outColor))
            material.metalness = outColor.r;
        if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, outColor))
            material.roughness = outColor.r;

        float emissiveIntensity = 0.0f;
        if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity))
            material.emission = emissiveIntensity;
        else if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, outColor))
            material.emission = std::max({ outColor.r, outColor.g, outColor.b });

        material.diffuseTexture   = ExtractTexture(path, assimpMaterial, scene, aiTextureType_DIFFUSE);
        material.aoTexture        = ExtractTexture(path, assimpMaterial, scene, aiTextureType_AMBIENT_OCCLUSION);
        material.metalnessTexture = ExtractTexture(path, assimpMaterial, scene, aiTextureType_METALNESS);
        material.roughnessTexture = ExtractTexture(path, assimpMaterial, scene, aiTextureType_DIFFUSE_ROUGHNESS);
        material.normalTexture    = ExtractTexture(path, assimpMaterial, scene, aiTextureType_NORMALS);
        material.emissionTexture  = ExtractTexture(path, assimpMaterial, scene, aiTextureType_EMISSION_COLOR);
        if (!material.emissionTexture.is_valid())
            material.emissionTexture = ExtractTexture(path, assimpMaterial, scene, aiTextureType_EMISSIVE);

        mMaterials[materialUUID] = material;
        return materialUUID;
    }

    Importer::TextureAsset Importer::TextureAsset::FromMemory(const uint8_t* bytes, size_t size)
    {
        int requiredChannels = 4; // Force RGBA
        int width, height, channels;
        uint8_t* image = stbi_load_from_memory(bytes, size, &width, &height, &channels, requiredChannels);
        if (!image)
        {
            Gep::Log::Error("Failed to load texture from raw data");
            return{};
        }

        TextureAsset tex = TextureAsset::FromPixels(image, width, height, requiredChannels);
        stbi_image_free(image);

        return tex;
    }

    Importer::TextureAsset Importer::TextureAsset::FromPixels(const uint8_t* pixelData, size_t width, size_t height, int requiredChannels)
    {
        TextureAsset tex;
        tex.size = { width, height };

        tex.bytes.resize(width * height);
        std::memcpy(tex.bytes.data(), pixelData, tex.bytes.size());

        return tex;
    }

    Importer::TextureAsset Importer::TextureAsset::FromFile(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            Gep::Log::Error("Cannot load texture: [", path.string(), "] does not exist");
            return {};
        }

        int width, height, channels;
        int required_channels = 4; // Force RGBA
        uint8_t* image = stbi_load(path.string().c_str(), &width, &height, &channels, required_channels);
        if (!image)
        {
            Gep::Log::Error("Failed to load texture: [", path.string(), "]");
            return {};
        }

        TextureAsset tex = TextureAsset::FromPixels(image, width, height, required_channels);
        stbi_image_free(image);

        return tex;

    }
}
