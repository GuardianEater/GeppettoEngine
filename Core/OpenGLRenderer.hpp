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
#include <unordered_map>

#include "stb_image.h"
#include "Logger.hpp"
#include "KeyedVector.hpp"
#include "BVHTree.hpp"
#include "IRenderTarget.hpp"

#include "Model.hpp"

#include "GPUVector.hpp"

 // fwd
struct aiScene;
struct aiMaterial;
enum aiTextureType;

namespace Gep
{
    struct alignas(16) MaterialGPUData
    {
        float ao = 0.8f;        // ambient occlusion. uniformly applied to the mesh. Will only be used if the ao texture handle is null
        float roughness = 0.8f; // diffuse roughness. uniformly applied to the mesh. Will only be used if the roughness texture handle is null
        float metalness = 0.8f; // uniformly applied to the mesh. Will only be used if the metalness texture handle is null
        float __pad;     // used for allignment

        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // diffuse color. uniformly applied to the mesh. Will only be used if the color texture handle is null
        
        GLuint64 aoTextureHandle = 0;        // 64 bit gpu pointer, used to sample ao texture on the gpu
        GLuint64 roughnessTextureHandle = 0; // 64 bit gpu pointer, used to sample roughness texture on the gpu
        GLuint64 metalnessTextureHandle = 0; // 64 bit gpu pointer, used to sample metalness texture on the gpu
        GLuint64 colorTextureHandle = 0;     // 64 bit gpu pointer, used to sample color texture on the gpu
        GLuint64 normalTextureHandle = 0;    // 64 bit gpu pointer, used to sample normal texture on the gpu
        GLuint64 padding;
    };

    struct alignas(16) ObjectGPUData
    {
        glm::mat4 modelMatrix;  // the location rotation and scale of an object; converts from a model from model space to world space

        // Store mat3 as 3 vec4 columns (w unused) to match std430 16-byte column stride
        glm::vec3 normalMatrixCol0; float pad0;
        glm::vec3 normalMatrixCol1; float pad1;
        glm::vec3 normalMatrixCol2; float pad2;

        int boneOffset; // should be added to this objects vertices boneindices to locate the correct bone matrices 
        int pad[3];
    };

    struct alignas(16) MeshGPUData
    {
        uint32_t materialIndex; // index into the materials ssbo

        int pad[3];
    };

    struct alignas(16) CameraGPUData
    {
        glm::mat4 perspectiveMatrix; // perspective matrix for camera
        glm::mat4 viewMatrix;        // view matrix for camera

        glm::vec4 camPosition; // position of the camera in world space
    };

    struct LightGPUData
    {
        glm::vec3 position; // location of the light in world space
        float pad; 

        glm::vec3 color; // color of the light
        float intensity; // intensity of the light
    };

    struct BoneGPUData
    {
        glm::mat4 offsetMatrix; // how the bone should move; used for animation
    };

    struct LineGPUData // this is not actually sent to the gpu
    {
        struct LineSegment
        {
            glm::vec3 start, end; // begining and end point in world space of the line
        };

        glm::vec3 color; // color of the line when drawn.
        std::vector<LineSegment> points; // this is the only data sent to the gpu per line
        // formula??
    };

    enum class RenderFlags : uint32_t
    {
        None = 0, // no render flags will do nothing special when drawing this object
        Wireframe = 1 << 0, // will draw this object with wireframe mode enabled
        Blending = 1 << 1, // [UNIMPLEMENTED] will draw this object with alpha blending enabled
        NoDepthTest = 1 << 2, // draws thisdiables depth test
        Highlight = 1 << 3, // [UNIMPLEMENTED] will draw this object highlighted
        NoBackfaceCull = 1 << 4, // wether or not to backface cull
    };

    // enable bitwise ops for the enum
    inline RenderFlags operator|(RenderFlags a, RenderFlags b)
    {
        return static_cast<RenderFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline RenderFlags operator&(RenderFlags a, RenderFlags b)
    {
        return static_cast<RenderFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline RenderFlags& operator|=(RenderFlags& a, RenderFlags b)
    {
        a = a | b;
        return a;
    }

    // hash for RenderFlags so it can be used as a key in unordered_map
    struct RenderFlagsHash
    {
        size_t operator()(RenderFlags f) const noexcept
        {
            return std::hash<uint32_t>()(static_cast<uint32_t>(f));
        }
    };

    class OpenGLRenderer
    {
    public:
        // adds a model directly from a file using its path as its name. necessary textures
        void AddModelFromFile(const std::string& path);

        // adds a prexisting model into the renderer. will not perform any loading from disk
        void AddModel(const std::string& name, const Gep::Model& model);

        void AddAnimation(const std::string& name, const Gep::Animation& animation);
        void AddMaterial(const Gep::Material& material);

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

        // currently done automatically with objects
        void CommitMeshes(); // moves all of the added mesh data from the cpu to the gpu
        void CommitMaterials(); // moves all of the added materials from the cpu to the gpu

        void SetCameraIndex(uint32_t index);
        void SetLightCount(uint32_t count);

        std::vector<std::string> GetLoadedMeshes() const;
        std::vector<std::filesystem::path> GetLoadedTextures() const;
        std::vector<std::string> GetLoadedAnimations() const;

        const std::vector<std::string>& GetSupportedModelFormats() const;
        const std::vector<std::string>& GetSupportedTextureFormats() const;

        void LoadIconTexture(const std::filesystem::path& iconPath);
        Texture GetIconTexture(const std::string& extension);
        Texture GetOrLoadIconTexture(const std::filesystem::path& iconPath);

        void LoadTextureAsync(const std::filesystem::path& texturePath);
        void LoadShader(const std::string& name, const std::filesystem::path& vert, const std::filesystem::path& frag);
        void ReloadShaders(); // Recompiles all shaders.

        // loads a texture from disk
        void LoadTexture(const std::filesystem::path& texturePath);

        // assuming the data is compressed, png/jpg
        void LoadTexture(const std::string& name, const uint8_t* imageFileData, size_t size);

        Texture GetTexture(const std::string& texturePath);
        Texture GetOrLoadTexture(const std::filesystem::path& texturePath);

        void LoadErrorTexture(const std::filesystem::path& texturePath);
        Texture GetErrorTexture() const;

        void Draw() const;

        void UnloadModel(const std::string& name);

        // Start must be called before rendering and End must be called after rendering
        void Start(const glm::vec3& color = { 0, 0, 0 });
        void End(); // resets the state of the renderer must be called after all draw calls

        void SetUpLineDrawing();
        glm::quat InterpolateRotation(const Track& track, float time);
        glm::vec3 InterpolatePosition(const Track& track, float time);
        glm::vec3 InterpolateScale(const Track& track, float time);

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
        void DrawRegular() const;
        void DrawLines() const;
        void AddWireframeObject(const std::string& modelName, const ObjectGPUData& objectData);

        // pixel data loaded from stbimage, note pixel data must be freed after use
        void LoadTextureFromPixelData(const std::string& name, const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);

        // helpers for loading assimp files
        Gep::Model LoadModelFromFile(const std::filesystem::path& path);
        void LoadMaterials(const std::filesystem::path& path, const aiScene* scene);

        void LoadAnimations(const std::string& name, Gep::Model& model, const aiScene* scene);

        // given information, will load textures onto the gpu that are needed by the given material. will return num_max<GLuint>() if there is no texture loaded
        Texture LoadTexturesFromAssimpMaterial(const std::filesystem::path& modelPath, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type);

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
        Gep::keyed_vector<Material> mMaterials;

        std::unordered_map<std::string, Gep::Texture> mIconTextures;// icon extension -> texture
        std::unordered_map<std::string, Gep::Texture> mTextures; // texture path -> texture

        Texture mErrorTexture{}; // always loaded, used when a texuture fails to load
        Material mErrorMaterial{};

        std::mutex mTextureLoadingMutex{};
    
        Gep::gpu_vector<ObjectGPUData,   0> mObjectUniforms;   // this vector is perfectly copied onto the gpu into the objectUniforms array
        Gep::gpu_vector<LightGPUData,    1> mLightUniforms;    // this vector is perfectly copied onto the gpu into the lights array
        Gep::gpu_vector<CameraGPUData,   2> mCameraUniforms;   // this vector is perfectly copied onto the gpu into the cameraUniforms array
        Gep::gpu_vector<BoneGPUData,     3> mBoneUniforms;     // this vector is perfectly copied onto the gpu into the boneUniforms array
        Gep::gpu_vector<MaterialGPUData, 4> mMaterialUniforms; // this vector is perfectly copied onto the gpu into the materialUniforms array
        Gep::gpu_vector<MeshGPUData,     5> mMeshUniforms;     // this vector is perfectly copied onto the gpu into the meshUniforms array

        // shader -> model -> flags -> objects
        std::map<std::string, std::map<std::string, std::map<RenderFlags, std::vector<ObjectGPUData>>>> mObjectDatas;

        // used to store vertices for drawing lines
        GLuint mLineVBO;
        GLuint mLineVAO;
        std::vector<LineGPUData> mLineUniforms;

    public:
        Gep::bvh_tree<uint64_t, Gep::Entity> mBVHTree;
    };
}
