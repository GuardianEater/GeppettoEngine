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
#include "Shader.hpp"

#include <mutex>
#include <unordered_map>

#include "stb_image.h"
#include "Logger.hpp"
#include "gtl/keyed_vector.hpp"

#include "Model.hpp"

#include "GPUVector.hpp"
#include "GPUKeyedVector.hpp"

#include "FrameBuffer.hpp"

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

    struct alignas(16) StaticObjectGPUData
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
        glm::mat4 pvMatrix;  // perspective view matrix for camera
        glm::mat4 ipvMatrix; // inverse perspective view matrix for camera

        glm::vec3 position; // position of the camera in world space
    };

    struct PointLightGPUData
    {
        glm::vec3 position; // location of the light in world space
        float pad; // used for shadow mapping, defines the far plane of the light's perspective projection

        glm::vec3 color; // color of the light
        float intensity; // intensity of the light

        glm::mat4 modelMatrix; // used for the bounding sphere
    };

    struct alignas(16) PointLightShadowGPUData
    {
        PointLightGPUData light{};

        glm::mat4 shadowMatrices[6]; // used for point light shadow mapping, each matrix corresponds to a face of the cubemap

        GLuint64 shadowMapHandle = NULL; // 64 bit gpu pointer, used to sample the shadow map on the gpu
        GLuint64 padding; // used for alignment
    };

    struct alignas(16) DirectionalLightGPUData
    {
        glm::vec3 position; // location of the light in world space
        float pad;

        glm::vec3 color; // color of the light
        float intensity; // intensity of the light

        glm::vec3 direction; // the direction of the light
        float pad0;
    };

    struct alignas(16) DirectionalLightShadowGPUData
    {
        DirectionalLightGPUData light;

        glm::mat4 pvMatrix;
        GLuint64 shadowMapHandle = NULL;
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
        // must be called after OpenGL context is created
        void Initialize();

        // adds a model directly from a file using its path as its name. necessary textures
        void AddModelFromFile(const std::string& path);

        // adds a prexisting model into the renderer. will not perform any loading from disk
        void AddModel(const std::string& name, const Gep::Model& model);

        void AddAnimation(const std::string& name, const Gep::Animation& animation);
        size_t AddMaterial(const Gep::MaterialGPUData& material);

        const Gep::Model& GetModel(const std::string& name);
        const Gep::Animation& GetAnimation(const std::string& name);

        bool IsAnimationLoaded(const std::string& name) const;
        bool IsModelLoaded(const std::string& name) const;

        // adds an object to be drawn by 
        void AddStaticObject(const std::string& shaderName, const std::string& modelName, const StaticObjectGPUData& objectData, RenderFlags flags = RenderFlags::None);
        void AddCamera(const CameraGPUData& cameraData);
        void AddPointLight(const PointLightGPUData& lightData); // adds a light to the renderered, will be sent to the shader when DrawLights is called
        void AddPointLightShadow(const PointLightShadowGPUData& lightData, const FrameBuffer& fbo); // variant of pointlight that will cast shadows
        void AddDirectionalLight(const DirectionalLightGPUData& uniforms);
        void AddDirectionalLightShadow(const DirectionalLightShadowGPUData& uniforms, const FrameBuffer& fbo);
        void AddBone(const BoneGPUData& boneData);
        void AddLine(const LineGPUData& lines); // adds a line set to be drawn

        void CommitObjects(); // moves all of the added object data from the cpu to the gpu
        void CommitCameras(); // moves all of the added camera data from the cpu to the gpu
        void CommitBones();   // moves all of the added bone data from the cpu to the gpu
        void CommitLights();  // moves all of the added light data from the cpu to the gpu

        void SetCameraIndex(uint32_t index);

        std::vector<std::string> GetLoadedModels() const;
        std::vector<std::filesystem::path> GetLoadedTextures() const;
        std::vector<std::string> GetLoadedAnimations() const;

        const std::vector<std::string>& GetSupportedModelFormats() const;
        const std::vector<std::string>& GetSupportedTextureFormats() const;

        void LoadIconTexture(const std::filesystem::path& iconPath);
        Texture GetIconTexture(const std::string& extension);
        Texture GetOrLoadIconTexture(const std::filesystem::path& iconPath);

        void LoadTextureAsync(const std::filesystem::path& texturePath);

        // shaders ///////////////////////////////////////////////////////////////////////////////////////////
        void ReloadShaders(); // Recompiles all shaders.

        // loads a texture from disk
        void LoadTexture(const std::filesystem::path& texturePath);

        // assuming the data is compressed, png/jpg
        void LoadTexture(const std::string& name, const uint8_t* imageFileData, size_t size);

        Texture GetTexture(const std::string& texturePath);
        Texture GetOrLoadTexture(const std::filesystem::path& texturePath);

        void LoadErrorTexture(const std::filesystem::path& texturePath);
        Texture GetErrorTexture() const;

        // draws all added objects to the given target framebuffer
        void Draw(Gep::FrameBuffer& targetFrameBuffer);

        void UnloadModel(const std::string& name);

        FrameBuffer& GetGeometryFrameBuffer() { return mGeometryFrameBuffer; }

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
            GLuint diffuseTexture = NumMax<GLuint>();
            GLuint aoTexture = NumMax<GLuint>();
            GLuint metalnessTexture = NumMax<GLuint>();
            GLuint roughnessTexture = NumMax<GLuint>();
        };

        struct MeshGPUHandle
        {
            void GenVertexBuffer(const Mesh& mesh);
            void GenIndexBuffer(const Mesh& mesh);
            void BindBuffers();
            void DeleteBuffers();

            // handles used by opengl
            GLuint mVertexArrayObject = NumMax<GLuint>();
            GLuint mVertexBuffer = NumMax<GLuint>();
            GLuint mIndexBuffer = NumMax<GLuint>();
            size_t mIndexCount{}; // the amount of indices in the index buffer
        };

        struct ModelGPUHandle
        {
            std::vector<MeshGPUHandle> meshHandles;
        };

        auto GetAllShaders()
        {
            return std::tie(
                mShader_GeometryStatic, 
                mShader_GeometrySkinned, 
                mShader_PointLight, 
                mShader_Line, 
                mShader_PointLightWithShadows, 
                mShader_PointLightShadowDepth,
                mShader_DirectionalLight,
                mShader_DirectionalLightWithShadows,
                mShader_DirectionalLightShadowDepth
            );
        }

    private:
        void GeometryPass(const Gep::FrameBuffer& targetFrameBuffer); // renders all geometry to the geometry framebuffer
        void PointLightPass(Gep::FrameBuffer& targetFrameBuffer);     // renders all point light emissions to the target framebuffer, but doesnt draw the light itself
        void PointLightShadowDepthPass(); // renders the depth map for each point light that casts shadows
        void DirectionalLightPass(Gep::FrameBuffer& targetFrameBuffer);
        void DirectionalLightShadowDepthPass(); // renders the depth map for each direcational light that casts shadows
        void DrawLines();
        void AddWireframeObject(const std::string& modelName, const StaticObjectGPUData& objectData);

        // pixel data loaded from stbimage, note pixel data must be freed after use
        void LoadTextureFromPixelData(const std::string& name, const uint8_t* pixelData, size_t width, size_t height, int requiredChannels);

        // helpers for loading assimp files
        Gep::Model LoadModelFromFile(const std::filesystem::path& path);
        void LoadMaterials(const std::filesystem::path& path, const aiScene* scene);

        void LoadAnimations(const std::string& name, Gep::Model& model, const aiScene* scene);

        // given information, will load textures onto the gpu that are needed by the given material. will return NumMax<GLuint>() if there is no texture loaded
        Texture LoadTexturesFromAssimpMaterial(const std::filesystem::path& modelPath, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type);

        void LoadAnimation(const std::string& parentPath, const aiAnimation* assimpAnimation, const Skeleton& skeleton);
    private:
        // when creating shaders make sure to add them to GetAllShaders
        Shader mShader_GeometryStatic;  // shader used for geometry pass of static models
        Shader mShader_GeometrySkinned; // shader used for geometry pass of animated models
        Shader mShader_Line;            // shader used for drawing lines

        Shader mShader_PointLight;            // shader used for simple point lights
        Shader mShader_PointLightWithShadows; // shader used for point lights that cast shadows
        Shader mShader_PointLightShadowDepth; // shader used to generate the depth cube map of shadow casting point lights

        Shader mShader_DirectionalLight;            // shader used for simple directional lights
        Shader mShader_DirectionalLightWithShadows; // shader used for directional lights that cast shadows
        Shader mShader_DirectionalLightShadowDepth; // shader used to generate the depth map of directional lights

        ComputeShader mShader_HorizontalBlur;
        ComputeShader mShader_VerticalBlur;

        glm::vec3 mSolidColor{};

        std::unordered_map<std::string, std::pair<ModelGPUHandle, Gep::Model>> mModels; // model name -> its handle and data
        std::unordered_map<std::string, Gep::Animation> mAnimations;

        std::unordered_map<std::string, Gep::Texture> mIconTextures;// icon extension -> texture
        std::unordered_map<std::string, Gep::Texture> mTextures; // texture path -> texture

        Texture mErrorTexture{}; // always loaded, used when a texuture fails to load
        Material mErrorMaterial{};

        std::mutex mTextureLoadingMutex{};

        FrameBuffer mGeometryFrameBuffer;

        struct ObjectDrawInfo
        {
            // the amount of objects to draw
            uint64_t count = 0;
            // vao and index count
            std::vector<std::pair<GLuint, size_t>> vaos; // all of the meshes to draw with that object
        };
    
        std::vector<ObjectDrawInfo> mStaticObjectDrawInfo; // synced with object uniforms stores addition meta information
        Gep::gpu_vector<StaticObjectGPUData, 0> mStaticObjectUniforms;          // copied into u_objects on the gpu
        Gep::gpu_vector<PointLightGPUData, 1> mPointLightUniforms;              // copied into u_pointLights on the gpu
        Gep::gpu_vector<CameraGPUData, 2> mCameraUniforms;                      // copied into u_cams on the gpu
        Gep::gpu_vector<BoneGPUData, 3> mBoneUniforms;                          // copied into u_bones on the gpu
        Gep::gpu_keyed_vector<MaterialGPUData, 4> mMaterials;                   // copied into u_materials on the gpu
        Gep::gpu_vector<MeshGPUData, 5> mMeshUniforms;                          // copied into u_meshes on the gpu
        Gep::gpu_vector<DirectionalLightGPUData, 6> mDirectionalLightUniforms;  // copied into u_directionalLights on the gpu
        Gep::gpu_vector<PointLightShadowGPUData, 7> mPointLightShadowUniforms;  // copied into u_pointLightShadows on the gpu
        Gep::gpu_vector<DirectionalLightShadowGPUData, 8> mDirectionalLightShadowUniforms;  // copied into u_pointLightShadows on the gpu

        std::vector<FrameBuffer> mPointLightShadowMaps; // index corresponds to the point light shadow uniform at the same index in mPointLightShadowUniforms
        std::vector<FrameBuffer> mDirectionalLightShadowMaps; // index corresponds to the directional light shadow uniform at the same index in mPointLightShadowUniforms

        // model -> flags -> objects
        std::map<std::string, std::map<RenderFlags, std::vector<StaticObjectGPUData>>> mObjectDatas;

        // used to store vertices for drawing lines
        GLuint mLineVBO;
        GLuint mLineVAO;
        std::vector<LineGPUData> mLineUniforms;
    };
}
