/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#include "pch.hpp"

#include "Renderer.hpp"

#include <stb_image.h>

namespace Gep
{
    Renderer::Renderer()
    {
        // basic setup
        glEnable(GL_CULL_FACE);

        // buffer setup
        BufferGen();
        BufferSetupAttributes();

        // ssbo setup
        SetupObjectSSBO();
        SetupLightSSBO();
        SetupCameraSSBO();
    }

    size_t Renderer::LoadShader(const Shader& newShader)
    {
        size_t newShaderDataID = mShaders.emplace();
        Shader_Internal& newShaderdata = mShaders.at(newShaderDataID);

        newShaderdata.shader = newShader;

        glGenBuffers(1, &newShaderdata.commandBuffer);
    }

    size_t Renderer::LoadTexture(const Texture& newTexture)
    {
        size_t newTextureDataID = mTextures.emplace();
        Texture_Internal& newTextureData = mTextures.at(newTextureDataID);

        newTextureData.texture = newTexture;

        glGenTextures(1, &newTextureData.handle);
        glBindTexture(GL_TEXTURE_2D, newTextureData.handle);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Ensure proper alignment
        GLenum format = (newTexture.channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, newTexture.width, newTexture.height, 0, format, GL_UNSIGNED_BYTE, newTexture.data.data());

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture

        return newTextureDataID;
    }

    size_t Renderer::LoadMaterial(const Material& newMaterial)
    {
        size_t newMaterialDataID = mMaterials.emplace();
        Material_Internal& newMaterialData = mMaterials.at(newMaterialDataID);
        newMaterialData.material = newMaterial;

        return newMaterialDataID;
    }

    size_t Renderer::LoadMesh(const Mesh& newMesh)
    {
        size_t newMeshDataIndex = mMeshes.emplace();
        Mesh_Internal& newMeshData = mMeshes.at(newMeshDataIndex);

        newMeshData.mesh = newMesh;

        // buffers for combining all vertices/indices
        std::vector<Vertex> allVertices;
        std::vector<uint32_t> allIndices;
        std::vector<DrawCommand> allCommands;

        // loop over all existing meshes and recompute offsets
        uint32_t currentVertexBufferOffset = 0;
        uint32_t currentIndexBufferOffset = 0;
        for (auto [index, meshData] : mMeshes)
        {
            allVertices.insert(allVertices.end(), meshData.mesh.vertices.begin(), meshData.mesh.vertices.end());
            
            for (uint32_t index : meshData.mesh.indices)
                allIndices.push_back(index + currentVertexBufferOffset); // correct the indices to point to the correct location in the combined vertex buffer

            currentVertexBufferOffset += meshData.mesh.vertices.size();
            currentIndexBufferOffset += meshData.mesh.indices.size();

            meshData.vertexBufferOffset = currentVertexBufferOffset;
            meshData.indexBufferOffset = currentIndexBufferOffset;
        }

        // upload new vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * allVertices.size(), allVertices.data(), GL_STATIC_DRAW);

        // upload new index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * allIndices.size(), allIndices.data(), GL_STATIC_DRAW);

        return newMeshDataIndex;
    }

    Renderer::KeyedVectorView<Mesh> Renderer::GetLoadedMeshes() const
    {
        KeyedVectorView<Mesh> view;
        view.reserve(mMeshes.size());

        for (const auto [index, mesh] : mMeshes)
            view.push_back({ index, &mesh.mesh });

        return view;
    }

    Renderer::KeyedVectorView<Shader> Renderer::GetLoadedShaders() const
    {
        KeyedVectorView<Shader> view;
        view.reserve(mShaders.size());

        for (const auto [index, shader] : mShaders)
            view.push_back({ index, &shader.shader });

        return view;
    }

    Renderer::KeyedVectorView<Texture> Renderer::GetLoadedTextures() const
    {
        KeyedVectorView<Texture> view;
        view.reserve(mTextures.size());

        for (const auto [index, texture] : mTextures)
            view.push_back({ index, &texture.texture });

        return view;
    }

    Renderer::KeyedVectorView<Material> Renderer::GetLoadedMaterials() const
    {
        KeyedVectorView<Material> view;
        view.reserve(mMaterials.size());

        for (const auto [index, material] : mMaterials)
            view.push_back({ index, &material.material });

        return view;
    }

    void Renderer::AddObject(const ObjectRenderInfo& info)
    {
        mShaders.at(info.shaderIndex).objectRenderInfos.push_back(info);

        Mesh_Internal& meshData = mMeshes.at(info.meshIndex);

        ++meshData.instanceCount;
    }

    void Renderer::AddLight(const LightRenderInfo& info)
    {
        mLightRenderInfos.push_back(info);
    }

    void Renderer::AddCamera(const CameraRenderInfo& info)
    {
        mCameraRenderInfos.push_back(info);
    }

    void Renderer::Render()
    {
        CommitLightInfo();
        CommitCameraInfo();

        glBindVertexArray(mVertexArrayObject);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mObjectSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, mLightSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mCameraSSBO);

        // sort objects by shader / when an object is added add it to its associated shader instead
        // one shader per object

        // for each camera
            // for each shader
                // for each object in this shader

        // make a clear call that clears all connected cameras

        int cameraIndex = 0;
        for (const auto& cameraInfo : mCameraRenderInfos)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, cameraInfo.frameBuffer);
            glViewport(0, 0, cameraInfo.frameBufferSize.x, cameraInfo.frameBufferSize.y);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto [index, shaderData] : mShaders)
            {
                shaderData.shader.Bind();
                CommitDrawCommands(shaderData);
                CommitObjectInfo(shaderData);
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, shaderData.commandBuffer);
                shaderData.shader.SetUniform(0, cameraIndex);

                glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, shaderData.drawCommandCount, 0);
            }

            ++cameraIndex;
        }
        
        glUseProgram(0);

        for (auto [index, shaderData] : mShaders)
            shaderData.objectRenderInfos.clear();

        // cleanup all of the frame data
        mLightRenderInfos.clear();
        mCameraRenderInfos.clear();
    }

    void Renderer::SetupObjectSSBO()
    {
        constexpr size_t initialCount = 1;
        constexpr size_t index = 0;

        glGenBuffers(1, &mObjectSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mObjectSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ObjectRenderInfo) * initialCount, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, mObjectSSBO);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
    }

    void Renderer::SetupLightSSBO()
    {
        constexpr size_t initialCount = 1;
        constexpr size_t index = 1;

        glGenBuffers(1, &mLightSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LightRenderInfo) * initialCount, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, mLightSSBO); // the number is the binding value of the buffer declared in the shader

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
    }

    void Renderer::SetupCameraSSBO()
    {
        constexpr size_t initialCount = 1;
        constexpr size_t index = 2;

        glGenBuffers(1, &mCameraSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mCameraSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CameraRenderInfo) * initialCount, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, mCameraSSBO);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
    }

    void Renderer::CommitObjectInfo(Shader_Internal& shader)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mObjectSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, shader.objectRenderInfos.size() * sizeof(ObjectRenderInfo), shader.objectRenderInfos.data(), GL_DYNAMIC_DRAW);
    }

    void Renderer::CommitLightInfo()
    {
        // sets the lightCount uniform in all of the shaders
        for (auto [index, shaderData] : mShaders)
            shaderData.shader.SetUniform(2, static_cast<int>(mLightRenderInfos.size()));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mLightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mLightRenderInfos.size() * sizeof(LightRenderInfo), mLightRenderInfos.data(), GL_DYNAMIC_DRAW);
    }

    void Renderer::CommitCameraInfo()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mCameraSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mCameraRenderInfos.size() * sizeof(CameraRenderInfo), mCameraRenderInfos.data(), GL_DYNAMIC_DRAW);
    }

    void Renderer::CommitDrawCommands(Shader_Internal& shaderData)
    {
        std::vector<DrawCommand> drawCommands;

        uint32_t currentBaseInstance = 0;
        for (auto [index, meshData] : mMeshes)
        {
            if (meshData.instanceCount == 0) continue;

            drawCommands.push_back({
                .count = static_cast<uint32_t>(meshData.mesh.indices.size()),
                .instanceCount = meshData.instanceCount,
                .firstIndex = meshData.indexBufferOffset,
                .baseVertex = meshData.vertexBufferOffset,
                .baseInstance = currentBaseInstance
            });

            currentBaseInstance += meshData.instanceCount;
        }

        shaderData.drawCommandCount = drawCommands.size();
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, shaderData.commandBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, drawCommands.size() * sizeof(DrawCommand), drawCommands.data(), GL_STATIC_DRAW);
    }

    void Renderer::BufferGen()
    {
        // creates the vertex array object
        glGenVertexArrays(1, &mVertexArrayObject);

        // creates the vertex/index buffer
        glGenBuffers(1, &mVertexBuffer);
        glGenBuffers(1, &mIndexBuffer);
    }

    void Renderer::BufferSetupAttributes() const
    {
        // setup the arribute array
        glBindVertexArray(mVertexArrayObject);
        {
            glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);        // binds the vertices to the array object
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer); // binds the indices to the array object

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
            glEnableVertexAttribArray(2);
        }
        glBindVertexArray(0);
    }
}
