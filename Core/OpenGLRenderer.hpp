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
#include "IRenderTarget.hpp"

#include "Model.hpp"

namespace Gep
{
    struct alignas(16) MaterialGPUData
    {
        float ao;        // ambient occlusion
        float roughness; 
        float metalness; float pad0;
        glm::vec3 color; float pad1;
    };

    struct alignas(16) ObjectGPUData
    {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;

        int isUsingTexture;
        int isIgnoringLight; 
        int isSolidColor;    
        int isWireframe;   
        MaterialGPUData material;
    };

    struct alignas(16) CameraGPUData
    {
        glm::mat4 perspectiveMatrix;
        glm::mat4 viewMatrix;

        glm::vec4 camPosition;
    };

    struct LightGPUData
    {
        glm::vec3 position; float pad;
        glm::vec3 color;   
        float intensity;
    };

    struct DrawCommand
    {
        std::string meshName;
        uint32_t instanceCount;
        uint32_t baseInstance;
    };

    class OpenGLRenderer
    {
    public:
        // adds a model directly from a file using its path as its name. necessary textures
        void AddModelFromFile(const std::string& path);

        // adds a prexisting model into the renderer. will not perform any loading from disk
        void AddModel(const std::string& name, const Gep::Model& model);

        bool IsMeshLoaded(const std::string& name) const;

        void SetShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);
        void SetHighlightShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);
        void SetColorShader(const std::filesystem::path& vert, const std::filesystem::path& frag);

        // adds an object to be drawn by 
        void AddObject(const std::string& modelName, const ObjectGPUData& objectData);
        void AddCamera(const CameraGPUData& cameraData);
        void AddLight(const LightGPUData& lightData); // adds a light to the renderered, will be sent to the shader when DrawLights is called

        void CommitObjects(); // moves all of the added object data from the cpu to the gpu
        void CommitCameras(); // moves all of the added camera data from the cpu to the gpu
        void CommitLights();  // moves all of the added light data from the cpu to the gpu

        void SetCameraIndex(size_t index);
        void SetLightCount(size_t count); 

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

        void Draw();

        void ToggleWireframes();
        void ToggleTextures();

        void UnloadModel(const std::string& name);
        void BackfaceCull(bool enabled = true);

        // Start must be called before rendering and End must be called after rendering
        void Start(const glm::vec3& color = { 0, 0, 0 });
        void End();

        void SetUpLightSSBO();
        void SetUpObjectUniformsSSBO();
        void SetUpCameraUniformsSSBO();
    private:
        struct MaterialGPUHandle
        {
            GLuint diffuseTexture = num_max<GLuint>();
            GLuint aoTexture = num_max<GLuint>();
        };

        struct MeshGPUHandle
        {
            void GenVertexBuffer(const Mesh& mesh);
            void GenIndexBuffer(const Mesh& mesh);
            void BindBuffers();
            void DeleteBuffers();

            // handles used by opengl
            GLuint mVertexArrayObject = num_max<GLuint>();
            GLuint mVertexBuffer = num_max<GLuint>();
            GLuint mIndexBuffer = num_max<GLuint>();
            size_t mIndexCount{}; // the amount of indices in the index buffer

            MaterialGPUHandle materialHandle{}; // handle to the material used by this mesh
        };

        struct ModelGPUHandle
        {
            std::vector<MeshGPUHandle> meshHandles;
            std::vector<ObjectGPUData> objectDatas;

            std::vector<ObjectGPUData> regularObjectDatas;
            std::vector<ObjectGPUData> wireframeObjectDatas;
        };

    private:
        void DrawRegular();
        void AddWireframeObject(const std::string& modelName, const ObjectGPUData& objectData);

    private:

        std::unique_ptr<Shader> mPBRShader;
        std::unique_ptr<Shader> mHighlightShader;
        std::unique_ptr<Shader> mColorShader;

        //keyed_vector<MeshGPUHandle> mModelHandles;
        bool mWireframeMode = false;
        bool mTexturesEnabled = true;

        glm::vec3 mSolidColor{};

        // camera
        glm::mat4 mNextPerspective{};
        glm::mat4 mNextView{};
        glm::vec4 mNextEye{};

        std::unordered_map<std::string, ModelGPUHandle> mModelHandles; // model name -> its handle
        size_t mTotalDrawCount;

        //std::unordered_map<std::string, MeshGPUHandle> mModelHandles;
        std::unordered_map<std::string, GLuint> mIconTextures;// icon extension -> texture id
        std::unordered_map<std::filesystem::path, GLuint> mTextures; // texture path -> texture id
        GLuint mErrorTexture{}; // always loaded, used when a texuture fails to load

        std::mutex mTextureLoadingMutex{};

        GLuint mLightUniformsSSBO{};
        std::vector<LightGPUData> mLightUniforms;

        GLuint mObjectsSSBO{};
        std::vector<ObjectGPUData> mObjectUniforms;

        GLuint mCameraUniformsSSBO{};
        std::vector<CameraGPUData> mCameraUniforms;

    public:
        Gep::bvh_tree<uint64_t, Gep::Entity> mBVHTree;
    };
}
