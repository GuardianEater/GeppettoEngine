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
    struct alignas(16) PBRMaterial
    {
        float ao;        // ambient occlusion
        float roughness; 
        float metalness; float pad0;
        glm::vec3 color; float pad1;
    };

    struct alignas(16) ObjectUniforms
    {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;

        int isUsingTexture;
        int isIgnoringLight; 
        int isSolidColor;    
        int isHighlighted;   
        PBRMaterial material;
    };

    struct alignas(16) CameraUniforms
    {
        glm::mat4 perspectiveMatrix;
        glm::mat4 viewMatrix;

        glm::vec4 camPosition;
    };

    struct LightUniforms
    {
        glm::vec3 position; float pad;
        glm::vec3 color;   
        float intensity;
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

        void SetShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);
        void SetHighlightShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);

        void AddObjectUniforms(const ObjectUniforms& uniforms);
        void AddCameraUniforms(const CameraUniforms& uniforms);
        void AddLightUniforms(const LightUniforms& uniforms); // adds a light to the renderered, will be sent to the shader when DrawLights is called

        void CommitObjectUniforms(); // moves all of the data from the cpu to the gpu
        void CommitCameraUniforms(); // moves all of the data from the cpu to the gpu
        void CommitLightUniforms(); // will send all added lights to the shader

        void SetObjectIndex(size_t index);
        void SetCameraIndex(size_t index);
        void SetLightCount(size_t count); 

        // changes how the renderer will draw the next object
        void SetTexture(GLuint texture);
        void SetHighlight(bool highlight);
        void SetWireframe(bool wireframe);

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


        void SetUpLightSSBO();
        void SetUpObjectUniformsSSBO();
        void SetUpCameraUniformsSSBO();
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

        std::unique_ptr<Shader> mActiveShader;
        std::unique_ptr<Shader> mHighlightShader;

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

        GLuint mLightUniformsSSBO{};
        std::vector<LightUniforms> mLightUniforms;

        GLuint mObjectUniformsSSBO{};
        std::vector<ObjectUniforms> mObjectUniforms;

        GLuint mCameraUniformsSSBO{};
        std::vector<CameraUniforms> mCameraUniforms;

        public:
        Gep::bvh_tree<uint64_t, Gep::Entity> mBVHTree;
    };
}
