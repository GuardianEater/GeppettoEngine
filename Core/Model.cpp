/*****************************************************************//**
 * \file   Model.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   June 2025
 *********************************************************************/

#include "pch.hpp"
#include "Model.hpp"

namespace Gep
{
    static void ProcessNode(const aiNode* node, const aiScene* scene, Gep::Mesh& mesh)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            // Convert vertices
            size_t vertex_offset = mesh.vertices.size();
            for (unsigned int v = 0; v < ai_mesh->mNumVertices; ++v)
            {
                Gep::Vertex vertex;

                vertex.position = {
                    ai_mesh->mVertices[v].x,
                    ai_mesh->mVertices[v].y,
                    ai_mesh->mVertices[v].z
                };

                vertex.normal = ai_mesh->HasNormals()
                    ? glm::vec3{ ai_mesh->mNormals[v].x, ai_mesh->mNormals[v].y, ai_mesh->mNormals[v].z }
                    : glm::vec3{ 0.0f, 0.0f, 0.0f };

                ai_mesh->mMaterialIndex;

                vertex.texCoord = (ai_mesh->HasTextureCoords(0))
                    ? glm::vec2{ ai_mesh->mTextureCoords[0][v].x, ai_mesh->mTextureCoords[0][v].y }
                    : glm::vec2{ 0.0f, 0.0f };

                mesh.vertices.push_back(vertex);
            }

            // Convert indices
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f)
            {
                const aiFace& face = ai_mesh->mFaces[f];

                mesh.indices.push_back(static_cast<uint32_t>(vertex_offset + face.mIndices[0]));
                mesh.indices.push_back(static_cast<uint32_t>(vertex_offset + face.mIndices[1]));
                mesh.indices.push_back(static_cast<uint32_t>(vertex_offset + face.mIndices[2]));
            }
        }

        // Recursively process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene, mesh);
        }
    }

    Model FromFile(const std::filesystem::path& path)
    {
        Model model;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path.string(),
            aiProcess_Triangulate |
            aiProcess_GenNormals |
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

        Gep::Mesh mesh;
        ProcessNode(scene->mRootNode, scene, mesh);

        mesh.Normalize();
        model.meshes.push_back(mesh);

        return model;
    }

    const std::vector<std::string>& SupportedExtensions()
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
