/*****************************************************************//**
 * \file   ModelSerializer.cpp
 * \brief  
 * 
 * \author 2018t
 * \date   April 2026
 *********************************************************************/

#include "pch.hpp"

#include "ModelSerializer.hpp"
#include "Conversion.hpp"

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
        if (!std::filesystem::exists(path))
        {
            Gep::Log::Error("Cannot import asset: [", path.string(), "] does not exist");
            return;
        }

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

        ExtractBoneInfo(scene); // needed before extracting either the skeleton or the mesh

        // loads all of the materials out of this scene
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            auto matUUID = ExtractMaterial(path, scene, i);
        }

        // fills in the skeleton of the model and the index field in gBoneData
        auto skelUUID = ExtractSkeleton(path, scene);

        auto modelUUID = ExtractModel(path, scene);

        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            auto animUUID = ExtractAnimation(path, scene, i);
        }
    }

    gtl::uuid Importer::ExtractTexture(const std::filesystem::path& path, const aiScene* scene, const aiMaterial* assimpMaterial, const aiTextureType type)
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
    gtl::uuid Importer::ExtractMaterial(const std::filesystem::path& path, const aiScene* scene, uint32_t assimpMaterialIdx)
    {
        const aiMaterial* assimpMat = scene->mMaterials[assimpMaterialIdx];
        const std::string matName = "m*" + std::to_string(assimpMaterialIdx);

        const auto it = mPathToUUID.find(matName);
        if (it != mPathToUUID.end())
            return it->second;

        MaterialAsset mat;
        gtl::uuid matUUID = gtl::generate_uuid();

        aiColor3D outColor(1.f, 1.f, 1.f);
        if (aiReturn_SUCCESS == assimpMat->Get(AI_MATKEY_COLOR_DIFFUSE, outColor))
            mat.color = { outColor.r, outColor.g, outColor.b, 1.0f };
        //if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, outColor))
        //    material.ao = outColor.r;
        if (aiReturn_SUCCESS == assimpMat->Get(AI_MATKEY_METALLIC_FACTOR, outColor))
            mat.metalness = outColor.r;
        if (aiReturn_SUCCESS == assimpMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, outColor))
            mat.roughness = outColor.r;

        float emissiveIntensity = 0.0f;
        if (aiReturn_SUCCESS == assimpMat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity))
            mat.emission = emissiveIntensity;
        else if (aiReturn_SUCCESS == assimpMat->Get(AI_MATKEY_COLOR_EMISSIVE, outColor))
            mat.emission = std::max({ outColor.r, outColor.g, outColor.b });

        mat.diffuseTexture   = ExtractTexture(path, scene, assimpMat, aiTextureType_DIFFUSE);
        mat.aoTexture        = ExtractTexture(path, scene, assimpMat, aiTextureType_AMBIENT_OCCLUSION);
        mat.metalnessTexture = ExtractTexture(path, scene, assimpMat, aiTextureType_METALNESS);
        mat.roughnessTexture = ExtractTexture(path, scene, assimpMat, aiTextureType_DIFFUSE_ROUGHNESS);
        mat.normalTexture    = ExtractTexture(path, scene, assimpMat, aiTextureType_NORMALS);
        mat.emissionTexture  = ExtractTexture(path, scene, assimpMat, aiTextureType_EMISSION_COLOR);
        if (!mat.emissionTexture.is_valid())
            mat.emissionTexture = ExtractTexture(path, scene, assimpMat, aiTextureType_EMISSIVE);

        mMaterials[matUUID] = mat;
        mPathToUUID[matName] = matUUID;
        return matUUID;
    }

    gtl::uuid Importer::ExtractSkeleton(const std::filesystem::path& path, const aiScene* scene)
    {
        // on the off chance a model doesn't have a root node?
        //uint32_t index = model.skeleton.bones.size();
        //Gep::Bone& bone = model.skeleton.bones.emplace_back();
        //bone.name = "Root";
        //bone.parentIndex = NumMax<uint32_t>();
        //bone.transformation = Gep::VQS{};
        //bone.inverseBind = Gep::VQS{};
        //bone.isRealBone = false;

        const std::string skelName = "s*" + path.string();

        const auto it = mPathToUUID.find(skelName);
        if (it != mPathToUUID.end())
            return it->second;

        Skeleton skel;
        gtl::uuid skelUUID = gtl::generate_uuid();

        FillHierarchyStep(skel, NumMax<uint32_t>(), scene->mRootNode);

        mSkeletons[skelUUID] = skel;
        mPathToUUID[skelName] = skelUUID;
        return skelUUID;
    }

    gtl::uuid Importer::ExtractAnimation(const std::filesystem::path& path, const aiScene* scene, uint32_t aAnimationIdx)
    {
        const std::string animName = "a*" + std::to_string(aAnimationIdx);

        const auto it = mPathToUUID.find(animName);
        if (it != mPathToUUID.end())
            return it->second;

        const gtl::uuid animUUID = gtl::generate_uuid();
        const aiAnimation* aAnimation = scene->mAnimations[aAnimationIdx];

        Animation anim;
        anim.duration = static_cast<float>(aAnimation->mDuration);
        anim.ticksPerSecond = aAnimation->mTicksPerSecond != 0.0 ? static_cast<float>(aAnimation->mTicksPerSecond) : 25.0f; // Assimp default

        anim.name = aAnimation->mName.C_Str();
        anim.tracks.reserve(aAnimation->mNumChannels);

        for (uint32_t i = 0; i < aAnimation->mNumChannels; i++)
        {
            const aiNodeAnim* channel = aAnimation->mChannels[i];

            //// find bone index in skeleton
            //auto it = std::find_if(skeleton.bones.begin(), skeleton.bones.end(), [&](const Bone& b)
            //    {
            //        return b.name == channel->mNodeName.C_Str();
            //    });

            //if (it == skeleton.bones.end())
            //{
            //    Gep::Log::Warning("Animation channel for bone '", channel->mNodeName.C_Str(), "' not found in skeleton");
            //    continue;
            //}

            //uint32_t boneIndex = static_cast<uint32_t>(std::distance(skeleton.bones.begin(), it));

            Track& track = anim.tracks.emplace_back();
            //track.boneIndex = boneIndex;

            // loop through all keyframes reading in their time and position
            track.positionKeyFrames.reserve(channel->mNumPositionKeys);
            track.rotationKeyFrames.reserve(channel->mNumRotationKeys);
            track.scaleKeyFrames.reserve(channel->mNumScalingKeys);

            for (size_t k = 0; k < channel->mNumPositionKeys; k++)
            {
                auto& keyFrame = track.positionKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mPositionKeys[k].mTime);
                keyFrame.transform = ToVec3(channel->mPositionKeys[k].mValue);
            }
            for (size_t k = 0; k < channel->mNumRotationKeys; k++)
            {
                auto& keyFrame = track.rotationKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mRotationKeys[k].mTime);
                keyFrame.transform = glm::normalize(ToQuat(channel->mRotationKeys[k].mValue));
            }
            for (size_t k = 0; k < channel->mNumScalingKeys; k++)
            {
                auto& keyFrame = track.scaleKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mScalingKeys[k].mTime);
                keyFrame.transform = ToVec3(channel->mScalingKeys[k].mValue);
            }
        }

        mAnimations[animUUID] = anim;
        mPathToUUID[animName] = animUUID;
        return animUUID;
    }

    void Importer::ExtractBoneInfo(const aiScene* scene)
    {
        // maps the name of every bone to its inverse bind transformation
        for (const aiMesh* mesh : std::span(scene->mMeshes, scene->mNumMeshes))
        {
            for (const aiBone* bone : std::span(mesh->mBones, mesh->mNumBones))
            {
                const std::string name = bone->mName.C_Str();
                mBoneInfos[name].offset = Gep::ToVQS(bone->mOffsetMatrix);
            }
        }
    }

    // returns the index of the node just created
    uint32_t Importer::FillHierarchyStep(Gep::Skeleton& skeleton, const uint32_t parentIndex, const aiNode* node)
    {
        // if the passed node is null return num max signaling that this is a leaf
        if (!node)
            return NumMax<uint32_t>();

        auto it = mBoneInfos.find(node->mName.C_Str());

        // if node is a bone sets it inverse bind otherwise leave as identity
        VQS inverseBind{};
        const bool isRealBone = it != mBoneInfos.end();
        if (isRealBone)
            inverseBind = it->second.offset;

        // create an entry in the heirarchy. 
        uint32_t index = skeleton.bones.size();
        Gep::Bone& bone = skeleton.bones.emplace_back();
        bone.name = node->mName.C_Str();
        bone.parentIndex = parentIndex;
        bone.transformation = ToVQS(node->mTransformation);
        bone.inverseBind = inverseBind;
        //bone.isRealBone = isRealBone;

        // if its a bone add the index to the name association. Used when extracting vertex weights
        if (isRealBone)
            it->second.index = index;

        // do the same thing for each child
        for (const aiNode* childNode : std::span(node->mChildren, node->mNumChildren))
        {
            uint32_t childIndex = FillHierarchyStep(skeleton, index, childNode);
            if (childIndex != NumMax<uint32_t>())
            {
                //note: cant get a reference here because it could be stale after recursive calls
                skeleton.bones.at(index).childrenIndices.push_back(childIndex);
            }
        }

        return index;
    }

    void Importer::FillVertices(Gep::Mesh& mesh, const aiMesh* aMesh)
    {
        mesh.vertices.reserve(aMesh->mNumVertices);

        for (uint32_t i = 0; i < aMesh->mNumVertices; ++i)
        {
            Vertex& v = mesh.vertices.emplace_back();

            v.position = { aMesh->mVertices[i].x, aMesh->mVertices[i].y, aMesh->mVertices[i].z };

            if (aMesh->HasNormals())
                v.normal = { aMesh->mNormals[i].x, aMesh->mNormals[i].y, aMesh->mNormals[i].z };

            if (aMesh->HasTextureCoords(0))
                v.texCoord = { aMesh->mTextureCoords[0][i].x, aMesh->mTextureCoords[0][i].y };
        }
    }

    void Importer::FillIndices(Gep::Mesh& mesh, const aiMesh* aMesh)
    {
        for (uint32_t i = 0; i < aMesh->mNumFaces; ++i)
        {
            const aiFace& face = aMesh->mFaces[i];

            for (uint32_t j = 0; j < face.mNumIndices; ++j)
                mesh.indices.push_back(face.mIndices[j]);
        }
    }

    gtl::uuid Importer::ExtractModel(const std::filesystem::path& path, const aiScene* scene)
    {
        const std::string modelName = "m*" + path.string();

        const auto it = mPathToUUID.find(modelName);
        if (it != mPathToUUID.end())
            return it->second;

        Model model;
        gtl::uuid modelUUID = gtl::generate_uuid();

        model.meshes.reserve(scene->mNumMeshes);

        for (const aiMesh* assimpMesh : std::span(scene->mMeshes, scene->mNumMeshes))
        {
            Mesh& mesh = model.meshes.emplace_back();
            mesh.name = assimpMesh->mName.C_Str();

            FillVertices(mesh, assimpMesh);
            FillIndices(mesh, assimpMesh);
            mesh.CalculateBoundingBox(); //must be done after vertices are loaded

            mesh.materialUUID = mPathToUUID.at("m*" + std::to_string(assimpMesh->mMaterialIndex));
            ExtractBoneWeightForVertices(mesh.vertices, assimpMesh, scene);
        }

        mModels[modelUUID] = model;
        mPathToUUID[modelName] = modelUUID;

        return modelUUID;
    }

    void Importer::SetVertexBoneData(Vertex& vertex, uint32_t boneID, float weight)
    {
        for (int i = 0; i < vertex.boneIndices.size(); ++i)
        {
            if (vertex.boneIndices[i] == Vertex::INVALID_INDEX)
            {
                vertex.boneWeights[i] = weight;
                vertex.boneIndices[i] = boneID;
                break;
            }
        }
    }

    void Importer::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* assimpMesh, const aiScene* scene)
    {
        for (const aiBone* assimpBone : std::span(assimpMesh->mBones, assimpMesh->mNumBones))
        {
            const std::string boneName = assimpBone->mName.C_Str();
            const uint32_t boneID = mBoneInfos.at(boneName).index; // index into the final bone heirarchy

            for (const aiVertexWeight assimpWeight : std::span(assimpBone->mWeights, assimpBone->mNumWeights))
            {
                const uint32_t vertexId = assimpWeight.mVertexId;
                const float weight = assimpWeight.mWeight;

                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
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
