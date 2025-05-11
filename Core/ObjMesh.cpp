/*****************************************************************//**
 * \file   ObjMesh.cpp
 * \brief  creates a mesh from an obj file
 * 
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   March 2025
 *********************************************************************/

#include "pch.hpp"

#include "ObjMesh.hpp"
#include "Mesh.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Gep
{

    static void ProcessNode(const aiNode* node, const aiScene* scene, Gep::Mesh& mesh)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            // Convert vertices
            size_t vertex_offset = mesh.mVertices.size();
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

                mesh.mVertices.push_back(vertex);
            }

            // Convert indices
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f)
            {
                const aiFace& face = ai_mesh->mFaces[f];
                if (face.mNumIndices == 3)
                {
                    mesh.mFaces.push_back({
                        static_cast<uint32_t>(vertex_offset + face.mIndices[0]),
                        static_cast<uint32_t>(vertex_offset + face.mIndices[1]),
                        static_cast<uint32_t>(vertex_offset + face.mIndices[2])
                        });
                }
            }
        }

        // Recursively process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene, mesh);
        }
    }

    Gep::Mesh LoadMesh(const std::filesystem::path& path)
    {
        Gep::Mesh mesh;

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

        ProcessNode(scene->mRootNode, scene, mesh);

        mesh.Normalize();
        return mesh;
    }

}
