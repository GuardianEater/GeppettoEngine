/*****************************************************************//**
 * \file   Renderer.hpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>
#include <glew.h>
#include <glm\glm.hpp>
#include <Mesh.hpp>
#include <Camera.hpp>
#include "Shader.hpp"

#include <mutex>

#include "stb_image.h"
#include "Logger.hpp"
#include "KeyedVector.hpp"
#include "BVHTree.hpp"

namespace Gep
{
    struct PBRMaterial
    {
        float ao; // ambient occlusion
        float roughness;
        float metalness;
        glm::vec3 color;
    };

    class OpenGLRenderer
    {
    public:

        // loads resources into the renderer
        void LoadMesh(const std::string& name, const Mesh& mesh);
        void LoadMesh(const std::filesystem::path& path); // path.string() will be used as the name
        uint64_t GetMesh(const std::string& name) const;
        uint64_t GetOrLoadMesh(const std::string& path); // if this is a predefined name it will still work. Must be given a path otherwise.

        bool IsMeshLoaded(const std::string& name) const;

        void SetShader(const std::string& name);

        // changes how the renderer will draw the next object
        void SetTexture(GLuint texture);
        void SetHighlight(bool highlight);
        void SetSolidColor(const glm::vec3& color);
        void SetIgnoreLight(bool ignore);
        void SetCamera(const Camera& camera);
        void SetCamera(const glm::mat4& pers, const glm::mat4& view, const glm::vec3& eye);
        void SetMaterial(const PBRMaterial& material);
        void SetModel(const glm::mat4& modelingMatrix);
        void SetWireframe(bool wireframe);
        void SetBackfaceCull(bool backfaceCull);
        std::vector<std::string> GetLoadedMeshes() const;
        std::vector<std::filesystem::path> GetLoadedTextures() const;

        const std::vector<std::string>& GetSupportedModelFormats() const;
        const std::vector<std::string>& GetSupportedTextureFormats() const;
        
        void LoadIconTexture(const std::filesystem::path& iconPath);
        GLuint GetIconTexture(const std::string& extension);
        GLuint GetOrLoadIconTexture(const std::filesystem::path& iconPath);

        void LoadTextureAsync(const std::filesystem::path& texturePath);
        void LoadTexture(const std::filesystem::path& texturePath);
        GLuint GetTexture(const std::filesystem::path& texturePath);
        GLuint GetOrLoadTexture(const std::filesystem::path& texturePath);

        void LoadErrorTexture(const std::filesystem::path& texturePath);
        GLuint GetErrorTexture() const;

        

        // completes the rendering of the object
        void DrawMesh(uint64_t meshID);

        void ToggleWireframes();
        void ToggleTextures();

        void UnloadMesh(const std::string& name);
        void BackfaceCull(bool enabled = true);

        // Start must be called before rendering and End must be called after rendering
        void Start(const glm::vec3& color = { 0, 0, 0 });
        void End();

        void AddLight(const glm::vec3& color, const glm::vec3& position, float intensity); // adds a light to the renderered, will be sent to the shader when DrawLights is called

        void DrawLights(); // will send all added lights to the shader
        void SetUpLightSSBO();

    private:
        struct MeshData
        {
            void GenVertexBuffer(const Mesh& mesh);
            void GenFaceBuffer(const Mesh& mesh);
            void BindBuffers();
            void DeleteBuffers();

            GLuint mVertexArrayObject = num_max<GLuint>();
            GLuint mVertexBuffer = num_max<GLuint>();
            GLuint mFaceBuffer = num_max<GLuint>();
            size_t mFaceCount{};
        };

    private:
        std::unordered_map<std::string, Shader> mShaders; // maps from the shader name, to the actual shader
        std::string mActiveShaderName = "";

        //keyed_vector<MeshData> mMeshDatas;
        bool mWireframeMode = false;
        bool mTexturesEnabled = true;

        bool mNextMeshIsBackfaceCulling = true; // if false, back faces will be drawn
        bool mNextMeshIsWireframe = false;
        bool mNextMeshIsHighlighted = false;
        bool mNextMeshIsSolidColor = false;
        glm::vec3 mSolidColor{};

        // camera
        glm::mat4 mNextPerspective{};
        glm::mat4 mNextView{};
        glm::vec4 mNextEye{};

        Gep::keyed_vector<MeshData> mMeshDatas;
        std::unordered_map<std::string, uint64_t> mMeshNameToID;

        //std::unordered_map<std::string, MeshData> mMeshDatas;
        std::unordered_map<std::string, GLuint> mIconTextures;// icon extension -> texture id
        std::unordered_map<std::filesystem::path, GLuint> mTextures; // texture path -> texture id
        GLuint mErrorTexture{}; // always loaded, used when a texuture fails to load

        std::mutex mTextureLoadingMutex{};

        struct LightData
        {
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec3 color;
            float intensity;
        };

        GLuint mLightSSBO{};
        std::vector<LightData> mLightData;

        public:
        Gep::bvh_tree<uint64_t, Gep::Entity> mBVHTree;
    };
}
