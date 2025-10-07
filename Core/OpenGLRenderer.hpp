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

 // fwd
struct aiScene;
struct aiMaterial;
enum aiTextureType;

namespace Gep
{
    struct alignas(16) MaterialGPUData
    {
        float ao;        // ambient occlusion
        float roughness;
        float metalness; float pad0;
        glm::vec4 color;
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

        int boneOffset; // should be added to this objects vertices boneindices to locate the correct bone matrices 
        int pad[3];
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

    struct BoneGPUData
    {
        glm::mat4 offsetMatrix;
    };

    struct LineGPUData // this is not actually sent to the gpu
    {
        struct LineSegment
        {
            glm::vec3 start, end;
        };

        glm::vec3 color;
        std::vector<LineSegment> points; // this is the only data sent to the gpu per line
        // formula??
    };

    enum class RenderFlags : uint32_t
    {
        None = 0,
        Wireframe = 1 << 0,
        Blending = 1 << 1,
        NoDepthTest = 1 << 2,
        // add more later if needed (Stencil, Cull, etc.)
    };

    // enable bitwise ops for the enum
    inline RenderFlags operator|(RenderFlags a, RenderFlags b)
    {
        return static_cast<RenderFlags>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
            );
    }

    inline RenderFlags operator&(RenderFlags a, RenderFlags b)
    {
        return static_cast<RenderFlags>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
            );
    }

    inline RenderFlags& operator|=(RenderFlags& a, RenderFlags b)
    {
        a = a | b;
        return a;
    }

    class OpenGLRenderer
    {
    public:
        // adds a model directly from a file using its path as its name. necessary textures
        void AddModelFromFile(const std::string& path);

        // adds a prexisting model into the renderer. will not perform any loading from disk
        void AddModel(const std::string& name, const Gep::Model& model);

        void AddAnimation(const std::string& name, const Gep::Animation& animation);

        const Gep::Model& GetModel(const std::string& name);
        const Gep::Animation& GetAnimation(const std::string& name);

        bool IsAnimationLoaded(const std::string& name) const;
        bool IsMeshLoaded(const std::string& name) const;
        bool IsShaderLoaded(const std::string& name) const;

        // adds an object to be drawn by 
        void AddObject(const std::string& shaderName, const std::string& modelName, const ObjectGPUData& objectData, RenderFlags flags = RenderFlags::None);
        void AddCamera(const CameraGPUData& cameraData);
        void AddLight(const LightGPUData& lightData); // adds a light to the renderered, will be sent to the shader when DrawLights is called
        void AddBone(const BoneGPUData& boneData);
        void AddLine(const LineGPUData& lines); // adds a line set to be drawn

        void CommitObjects(); // moves all of the added object data from the cpu to the gpu
        void CommitCameras(); // moves all of the added camera data from the cpu to the gpu
        void CommitBones();   // moves all of the added bone data from the cpu to the gpu
        void CommitLights();  // moves all of the added light data from the cpu to the gpu

        void SetCameraIndex(size_t index);
        void SetLightCount(size_t count);

        std::vector<std::string> GetLoadedMeshes() const;
        std::vector<std::filesystem::path> GetLoadedTextures() const;
        std::vector<std::string> GetLoadedAnimations() const;

        const std::vector<std::string>& GetSupportedModelFormats() const;
        const std::vector<std::string>& GetSupportedTextureFormats() const;

        void LoadIconTexture(const std::filesystem::path& iconPath);
        GLuint GetIconTexture(const std::string& extension);
        GLuint GetOrLoadIconTexture(const std::filesystem::path& iconPath);

        void LoadTextureAsync(const std::filesystem::path& texturePath);
        void LoadShader(const std::string& name, const std::filesystem::path& vert, const std::filesystem::path& frag);

        // loads a texture from disk
        void LoadTexture(const std::filesystem::path& texturePath);

        // assuming the data is compressed, png/jpg
        void LoadTexture(const std::string& name, const uint8_t* imageFileData, size_t size); // note destroys 

        GLuint GetTexture(const std::string& texturePath);
        GLuint GetOrLoadTexture(const std::filesystem::path& texturePath);

        void LoadErrorTexture(const std::filesystem::path& texturePath);
        GLuint GetErrorTexture() const;

        void Draw();

        void UnloadModel(const std::string& name);
        void BackfaceCull(bool enabled = true);

        // Start must be called before rendering and End must be called after rendering
        void Start(const glm::vec3& color = { 0, 0, 0 });
        void End();

        void SetUpLightSSBO();
        void SetUpObjectUniformsSSBO();
        void SetUpCameraUniformsSSBO();
        void SetUpBoneUniformsSSBO();

        void SetUpLineDrawing();
        Gep::VQS Interpolate(const Track& track, float time);
    private:
        struct MaterialGPUHandle
        {
            GLuint diffuseTexture = num_max<GLuint>();
            GLuint aoTexture = num_max<GLuint>();
            GLuint metalnessTexture = num_max<GLuint>();
            GLuint roughnessTexture = num_max<GLuint>();
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
        };

        struct AnimationGPUHandle
        {

        };


        struct ObjectCPUData // meta data for objects that is only needed on the cpu. corresponds to the gpu data variant
        {
            std::string shader;
            std::string model;
        };

        struct ObjectData
        {
            ObjectGPUData gpuData;
            ObjectCPUData cpuData;;
        };



    private:
        void DrawRegular();
        void DrawLines();
        void AddWireframeObject(const std::string& modelName, const ObjectGPUData& objectData);

        // pixel data loaded from stbimage, note pixel data must be freed after use
        void LoadTextureFromPixelData(const std::string& name, const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);

        // helpers for loading assimp files
        Gep::Model LoadModelFromFile(const std::filesystem::path& path);
        void LoadMaterials(Gep::Model& model, const std::filesystem::path& path, const aiScene* scene);

        void LoadAnimations(const std::string& name, Gep::Model& model, const aiScene* scene);

        // given information, will load textures onto the gpu that are needed by the given material. will return num_max<GLuint>() if there is no texture loaded
        GLuint LoadMaterial(const std::filesystem::path& modelPath, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type);

        void LoadAnimation(const std::string& parentPath, const aiAnimation* assimpAnimation, const Skeleton& skeleton);
    private:
        // name of the shader to the compiled shader
        std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;

        glm::vec3 mSolidColor{};

        // camera
        glm::mat4 mNextPerspective{};
        glm::mat4 mNextView{};
        glm::vec4 mNextEye{};

        std::unordered_map<std::string, std::pair<ModelGPUHandle, Gep::Model>> mModels; // model name -> its handle and data
        std::unordered_map<std::string, std::pair<AnimationGPUHandle, Gep::Animation>> mAnimations;

        //std::unordered_map<std::string, MeshGPUHandle> mModels;
        std::unordered_map<std::string, GLuint> mIconTextures;// icon extension -> texture id
        std::unordered_map<std::string, GLuint> mTextures; // texture path -> texture id
        GLuint mErrorTexture{}; // always loaded, used when a texuture fails to load

        std::mutex mTextureLoadingMutex{};

        GLuint mLightUniformsSSBO{};
        std::vector<LightGPUData> mLightUniforms;

        GLuint mObjectsSSBO{};
        std::vector<ObjectGPUData> mObjectUniforms;

        GLuint mCameraUniformsSSBO{};
        std::vector<CameraGPUData> mCameraUniforms;

        GLuint mBoneUniformsSSBO{};
        std::vector<BoneGPUData> mBoneUniforms;

        // sorted by shader name -> model name
        //std::vector<ObjectGPUData> mObjectsToRender;
        //std::vector<ObjectCPUData> mObjectsToRenderMeta;
        // shader name -> model name -> render state -> objects
        std::map<std::string, std::map<std::string, std::map<RenderFlags, std::vector<ObjectGPUData>>>> mObjectDatas;

        // used to store vertices for drawing lines
        GLuint mLineVBO;
        GLuint mLineVAO;
        std::vector<LineGPUData> mLineUniforms;

    public:
        Gep::bvh_tree<uint64_t, Gep::Entity> mBVHTree;
    };
}
