/*****************************************************************//**
 * \file   Model.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#include "pch.hpp"
#include "Model.hpp"
#include "Core.hpp"
#include "Conversion.h"

namespace Gep
{
    struct BoneInfo
    {
        int id;
        glm::mat4 offset;
    };

    static std::unordered_map<std::string, BoneInfo> gBoneInfoMap; //
    static std::unordered_map<std::string, VQS> gBoneData;
    static int gBoneCounter = 0;

    // moves all data from the aiScene into the internal model format
    static void LoadMaterials(Gep::Model& model, const aiScene* scene)
    {
        model.materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            Material& material = model.materials.emplace_back();
            aiMaterial* assimpMaterial = scene->mMaterials[i];

            aiColor3D diffuseColor(1.f, 1.f, 1.f);
            if (aiReturn_SUCCESS == assimpMaterial->Get("$clr.diffuse", 0, 0, diffuseColor))
                material.color = { diffuseColor.r, diffuseColor.g, diffuseColor.b };

            aiString texPath;
            if (aiReturn_SUCCESS == assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
                material.diffuseTexturePath = texPath.C_Str();
        }
    }

    // normalizes a model so it fits inside of a unit cube
    static void NormalizeModel(Gep::Model& model)
    {

    }

    static void SetVertexBoneDataToDefault(Vertex& vertex)
    {
        for (int i = 0; i < vertex.boneWeights.size(); i++)
        {
            vertex.boneIndices[i] = -1;
            vertex.boneWeights[i] = 0.0f;
        }
    }

    static void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < vertex.boneWeights.size(); ++i)
        {
            if (vertex.boneIndices[i] < 0)
            {
                vertex.boneWeights[i] = weight;
                vertex.boneIndices[i] = boneID;
                break;
            }
        }
    }

    static void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* assimpMesh)
    {
        for (int boneIndex = 0; boneIndex < assimpMesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = assimpMesh->mBones[boneIndex]->mName.C_Str();
            if (gBoneInfoMap.find(boneName) == gBoneInfoMap.end())
            {
                BoneInfo newBoneInfo{};
                newBoneInfo.id = gBoneCounter;
                newBoneInfo.offset = ToMat4(assimpMesh->mBones[boneIndex]->mOffsetMatrix);
                gBoneInfoMap[boneName] = newBoneInfo;
                boneID = gBoneCounter;
                ++gBoneCounter;
            }
            else
            {
                boneID = gBoneInfoMap[boneName].id;
            }

            assert(boneID != -1);
            auto weights = assimpMesh->mBones[boneIndex]->mWeights;
            int numWeights = assimpMesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());

                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }

    static void LoadVertices(Gep::Mesh& mesh, const aiMesh* assimpMesh)
    {
        mesh.vertices.reserve(assimpMesh->mNumVertices);

        for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i) 
        {
            Vertex& v = mesh.vertices.emplace_back();

            v.position = { assimpMesh->mVertices[i].x, assimpMesh->mVertices[i].y, assimpMesh->mVertices[i].z };

            if (assimpMesh->HasNormals())
                v.normal = { assimpMesh->mNormals[i].x, assimpMesh->mNormals[i].y, assimpMesh->mNormals[i].z };

            if (assimpMesh->HasTextureCoords(0))
                v.texCoord = { assimpMesh->mTextureCoords[0][i].x, assimpMesh->mTextureCoords[0][i].y };
        }

        ExtractBoneWeightForVertices(mesh.vertices, assimpMesh);
    }

    static void LoadIndices(Gep::Mesh& mesh, const aiMesh* assimpMesh)
    {
        for (unsigned int i = 0; i < assimpMesh->mNumFaces; ++i) 
        {
            const aiFace& face = assimpMesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                mesh.indices.push_back(face.mIndices[j]);
        }
    }

    // returns the index of the node just created
    static size_t LoadHierarchyStep(Gep::Model& model, size_t parentIndex, const aiNode* node)
    {
        if (!node)
            return num_max<size_t>();

        auto it = gBoneData.find(node->mName.C_Str());

        // if node is not a bone, skip adding it
        if (it == gBoneData.end())
        {
            // but still traverse children, since bones might be deeper
            size_t lastValid = num_max<size_t>();
            for (size_t i = 0; i < node->mNumChildren; ++i)
            {
                lastValid = LoadHierarchyStep(model, parentIndex, node->mChildren[i]);
            }
            return lastValid;
        }

        const auto& [boneName, inverseBind] = *it;

        // this is a bone, so add it, note cannot get a reference here because it could be stale
        size_t index = model.skeleton.bones.size();
        model.skeleton.bones.emplace_back();
        model.skeleton.bones.at(index).name = boneName;
        model.skeleton.bones.at(index).parentIndex = parentIndex;
        model.skeleton.bones.at(index).transformation = ToVQS(node->mTransformation);
        model.skeleton.bones.at(index).inverseBind = inverseBind; // use oinverse bind

        for (size_t i = 0; i < node->mNumChildren; ++i)
        {
            size_t childIndex = LoadHierarchyStep(model, index, node->mChildren[i]);
            if (childIndex != num_max<size_t>())
                model.skeleton.bones.at(index).childrenIndices.push_back(childIndex);
        }

        return index;
    }

    static void LoadHierarchy(Gep::Model& model, const aiScene* scene)
    {
        LoadHierarchyStep(model, num_max<size_t>(), scene->mRootNode);
    }

    static void LoadMeshes(Gep::Model& model, const aiScene* scene)
    {
        model.meshes.reserve(scene->mNumMeshes);

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            Mesh& mesh = model.meshes.emplace_back();

            LoadVertices(mesh, scene->mMeshes[i]);
            LoadIndices(mesh, scene->mMeshes[i]);

            mesh.materialIndex = scene->mMeshes[i]->mMaterialIndex;
        }
    }
    
    static void LoadBoneData(const aiScene* scene)
    {
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) 
        {
            aiMesh* mesh = scene->mMeshes[m];
            for (unsigned int b = 0; b < mesh->mNumBones; ++b) 
            {
                aiBone* bone = mesh->mBones[b];
                gBoneData[bone->mName.C_Str()] = ToVQS(bone->mOffsetMatrix);
            }
        }
    }

    Model Model::FromFile(const std::filesystem::path & path)
    {
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

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Gep::Log::Error("Assimp error: ", importer.GetErrorString());
            return {};
        }

        Gep::Model model;

        LoadBoneData(scene);
        LoadMeshes(model, scene);
        LoadMaterials(model, scene);
        LoadHierarchy(model, scene);

        return model;
    }

    const std::vector<std::string>& Model::SupportedExtensions()
    {
        static std::vector<std::string> allowedExtensions = []() // initializes this vector with the extensions that work with assimp
        {
            std::string s;
            Assimp::Importer importer;
            importer.GetExtensionList(s);

            s.erase(std::remove(s.begin(), s.end(), '*'), s.end());

            std::vector<std::string> out;
            std::istringstream ss(s);
            std::string token;
            while (std::getline(ss, token, ';'))
                if (!token.empty())
                    out.emplace_back(std::move(token));
            return out;
        }();

        return allowedExtensions;
    }
}
