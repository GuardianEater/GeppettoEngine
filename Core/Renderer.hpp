/*****************************************************************//**
 * \file   Renderer.hpp
 * \brief  Handles rendering using the GPU
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

// external
#include <glm/glm.hpp>
#include <limits>

// rendering
#include "Shader.hpp"
#include "KeyedVector.hpp"

namespace Gep
{
    constexpr size_t INVALID_TEXTURE = std::numeric_limits<GLuint>::max();

    class Renderer
    {
    public:
        // gpu
        struct alignas(16) PBRMaterial
        {
            glm::vec3 color; GLuint colorTexture;
            float ao;        GLuint aoTexture;  // ambient occlusion
            float roughness; GLuint roughnessTexture;
            float metalness; GLuint metalnessTexture;
        };

        // gpu
        struct alignas(16) ObjectRenderInfo
        {
            glm::mat4 modelMatrix;

            size_t materialIndex = 0;
            size_t meshIndex = 0;
            size_t shaderIndex = 0;

            int isUsingTexture = 0;
            int isIgnoringLight = 0;
            int isSolidColor = 0;
            int isHighlighted = 0;
        };

        // gpu
        struct alignas(16) CameraRenderInfo
        {
            glm::mat4 perspectiveMatrix;
            glm::mat4 viewMatrix;
            glm::vec3 camPosition;

            GLuint frameBuffer;
            glm::u32vec2 frameBufferSize;
        };

        // gpu
        struct alignas(16) LightRenderInfo
        {
            glm::vec3 position; float pad;
            glm::vec3 color;
            float intensity;
        };

        // gpu
        struct Vertex
        {
            glm::vec3 position{};
            glm::vec3 normal{};
            glm::vec2 texCoord{};
        };

        // cpu
        struct Mesh
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
        };

        // cpu
        struct Texture
        {
            static Texture FromFile(const std::filesystem::path& path);

            std::vector<uint8_t> data;

            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t channels = 0;
        };

    public:
        Renderer();

        size_t LoadShader(const Shader&);
        size_t LoadTexture(const Texture&);
        size_t LoadMaterial(const PBRMaterial&);
        size_t LoadMesh(const Mesh&);

        void AddObject(const ObjectRenderInfo&); // contains model, material, etc
        void AddLight(const LightRenderInfo&);   // contains position, brightness, etc
        void AddCamera(const CameraRenderInfo&); // contains perspective, view, etc

        void Render(); // draws to a given texture
        
    private:
        struct DrawCommand
        {
            GLuint count;
            GLuint instanceCount;
            GLuint firstIndex;
            GLuint baseVertex;
            GLuint baseInstance;
        };

        struct Mesh_Internal
        {
            Mesh mesh; // the mesh data that was given

            uint32_t indexBufferOffset{};  // the index into the indices array
            uint32_t vertexBufferOffset{}; // the index into the vertices array
            uint32_t instanceCount{};      // the amount of this mesh to be rendered
        };

        struct Shader_Internal
        {
            Shader shader; // the shader that was given

            GLuint commandBuffer = 0;    // opengl handle, contains all draw commands, generated for each mesh
            size_t drawCommandCount = 0; // the amount of draw commands 
        };

        struct Texture_Internal
        {
            Texture texure;

            GLuint handle;
        };

    private:
        void SetupObjectSSBO(); // prepares mObjectSSBO to be used by its associated commit function
        void SetupLightSSBO();  // prepares mLightSSBO to be used by its associated commit function
        void SetupCameraSSBO(); // prepares mCameraSSBO to be used by its associated commit function

        void CommitObjectInfo();   // sends all collected object info to the gpu
        void CommitLightInfo();    // sends all collected light info to the gpu
        void CommitCameraInfo();   // sends all collected camera info to the gpu
        void CommitDrawCommands(Shader_Internal& shaderData); // sends all of the collected draw commands to the gpu

        void BufferGen();                   // generates index/vertex buffers and vao
        void BufferSetupAttributes() const; // sets up the vao

    private:
        Gep::keyed_vector<Mesh_Internal> mMeshes;      // all of the mesh data that has been loaded
        Gep::keyed_vector<Shader_Internal> mShaders;   // all of the shader programs that have been loaded
        Gep::keyed_vector<PBRMaterial> mMaterials;     // all of the materials that have been loaded
        Gep::keyed_vector<Texture_Internal> mTextures; // all of the textures that have been loaded

        std::vector<ObjectRenderInfo> mObjectRenderInfos; // all objects the are queued to be drawn, passed directly to the gpu
        std::vector<LightRenderInfo>  mLightRenderInfos;  // all lights that are queued to be drawn, passed directly to the gpu
        std::vector<CameraRenderInfo> mCameraRenderInfos; // all cameras that are queued to be drawn to, passed directly to the gpu

        GLuint mVertexBuffer{};      // opengl handle, contains all of the vertices for all of the meshes
        GLuint mIndexBuffer{};       // opengl handle, contains all of the indecies for all of the meshes
        GLuint mVertexArrayObject{}; // opengl handle, contains both the index and vertex buffer
        GLuint mCommandBuffer{};     // opengl handle, contains all draw commands generated for each mesh

        GLuint mObjectSSBO{}; // gpu handle to the block of memory containing information on all objects
        GLuint mLightSSBO{};  // gpu handle to the block of memory containing information on all lights
        GLuint mCameraSSBO{}; // gpu handle to the block of memory containing information on all lights
    };
}
