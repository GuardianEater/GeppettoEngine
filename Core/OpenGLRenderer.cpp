/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  Base interface for the type of rendering being performed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#include "pch.hpp"

#include "OpenGLRenderer.hpp"
#include "Model.hpp"
#include "Conversion.hpp"
#include "VQS.hpp"

#include "SkyboxMesh.hpp"
#include "QuadMesh.hpp"
#include "SphereMesh.hpp"
#include "IcosphereMesh.hpp"
#include "CubeMesh.hpp"


#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "shellapi.h"
#undef LoadImage
#undef min
#undef max

namespace Gep
{
    struct GLDrawFlags
    {
        std::optional<std::pair<GLenum, GLenum>> depthFuncMask;
        std::optional<GLenum> cullMode;
        std::optional<std::pair<GLenum, GLenum>> blendFuncSD;
    };

    static void SetDrawFlags(GLDrawFlags flags)
    {
        if (flags.depthFuncMask)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(flags.depthFuncMask->first);
            glDepthMask(flags.depthFuncMask->second);
        }
        else
            glDisable(GL_DEPTH_TEST);

        if (flags.cullMode)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(*flags.cullMode);
        }
        else
            glDisable(GL_CULL_FACE);

        if (flags.blendFuncSD)
        {
            glEnable(GL_BLEND);
            glBlendFunc(flags.blendFuncSD->first, flags.blendFuncSD->second);
        }
        else
            glDisable(GL_BLEND);
    }

    enum GLVertexAttributeLocation : GLint
    {
        Position,
        Normal,
        TexCoord
    };
    enum GLUniformLocation : GLint
    {
        Perspective, // perspective projection matrix
        ViewMatrix,
        ModelMatrix,
        NormalMatrix,
        Eye,
        DiffuseCoefficient,
        SpecularCoefficient,
        SpecularExponent,
        AmbientColor,
        TextureSampler,
        UseTexture,

        LightCount,
        IsSolidColor,
        SolidColor,

        IsHighlighted,
        IgnoreLight,
    };

    void OpenGLRenderer::Initialize()
    {
        SetUpLineDrawing();

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        // gbuffer
        mGeometryFrameBuffer = FrameBuffer::Create({128, 128});
        mGeometryFrameBuffer.AddTexture(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT); // depth
        mGeometryFrameBuffer.AddTexture(GL_COLOR_ATTACHMENT0, GL_RGB16F, GL_RGB, GL_FLOAT); // normal
        mGeometryFrameBuffer.AddTexture(GL_COLOR_ATTACHMENT1, GL_RGBA8, GL_RGBA, GL_FLOAT); // color
        mGeometryFrameBuffer.AddTexture(GL_COLOR_ATTACHMENT2, GL_RGB8, GL_RGB, GL_FLOAT); // ao + roughness + metalness

        // setup geometry shaders
        mShader_Geometry  = Shader::FromFile("shaders/Geometry.vert",  "shaders/Geometry.frag");
        mShader_Line      = Shader::FromFile("shaders/Line.vert", "shaders/Line.frag");

        // setup pointlight shaders
        mShader_PointLight            = Shader::FromFile("shaders/Point/Lighting-Point.vert",  "shaders/Point/Lighting-Point.frag");
        mShader_PointLightWithShadows = Shader::FromFile("shaders/Point/Lighting-Point-Shaded.vert", "shaders/Point/Lighting-Point-Shaded.frag");
        mShader_PointLightShadowDepth = Shader::FromFile("shaders/Point/Shadows-Point.vert",   "shaders/Point/Shadows-Point.frag", "shaders/Point/Shadows-Point.geom");

        // setup directional light shaders
        mShader_DirectionalLight            = Shader::FromFile("shaders/Directional/Lighting-Directional.vert", "shaders/Directional/Lighting-Directional.frag");
        mShader_DirectionalLightWithShadows = Shader::FromFile("shaders/Directional/Lighting-Directional.vert", "shaders/Directional/Lighting-Directional-Shaded.frag");
        mShader_DirectionalLightShadowDepth = Shader::FromFile("shaders/Directional/Shadows-Directional.vert",  "shaders/Directional/Shadows-Directional.frag");

        Gep::Material defaultMat{};
        uint64_t defaultMatIdx = AddMaterial(defaultMat);

        // load all of the default meshes
        {
            Gep::Mesh quad = Gep::QuadMesh();
            quad.materialIndex = defaultMatIdx;
            quad.name = "Quad";
            Gep::Model model;
            model.name = "Quad";
            model.meshes.push_back(quad);
            AddModel(model);
        }
        {
            Gep::Mesh sphere = Gep::SphereMesh(10, 10);
            sphere.materialIndex = defaultMatIdx;
            sphere.name = "Sphere";
            Gep::Model model;
            model.name = "Sphere";
            model.meshes.push_back(sphere);
            uint64_t modelIdx = AddModel(model);
            mSphereMeshIndex = GetModelMeshes(modelIdx)[0];
        }
        {
            Gep::Mesh cube = Gep::CubeMesh();
            cube.materialIndex = defaultMatIdx;
            cube.name = "Cube";
            Gep::Model model;
            model.name = "Cube";
            model.meshes.push_back(cube);
            uint64_t modelIdx = AddModel(model);
            mCubeMeshIndex = GetModelMeshes(modelIdx)[0];
        }
        {
            Gep::Mesh icosphere = Gep::IcosphereMesh(3);
            icosphere.materialIndex = defaultMatIdx;
            icosphere.name = "Icosphere";
            Gep::Model model;
            model.name = "Icosphere";
            model.meshes.push_back(icosphere);
            AddModel(model);
        }
        {
            Gep::Mesh skybox = Gep::SkyboxMesh();
            skybox.materialIndex = defaultMatIdx;
            skybox.name = "Skybox";
            Gep::Model model;
            model.name = "Skybox";
            model.meshes.push_back(skybox);
            AddModel(model);
        }

        //// setup skybox
        mShader_EquirectangularToCubemap = Shader::FromFile("shaders/IBL/Cubemap.vert", "shaders/IBL/Equirectangular-To-Cubemap.frag");
        mShader_EquirectangularToCubemap.SetUniform("u_equirectangularMap", 0);

        mShader_CubemapToEquirectangular = Shader::FromFile("shaders/IBL/Cubemap-To-Equirectangular.vert", "shaders/IBL/Cubemap-To-Equirectangular.frag");
        mShader_CubemapToEquirectangular.SetUniform("u_cubeMap", 0);

        mShader_Background = Shader::FromFile("shaders/IBL/Background.vert", "shaders/IBL/Background.frag");
        mShader_Background.SetUniform("u_environmentMap", 0);

        mShader_AmbientLight = Shader::FromFile("shaders/Ambient.vert", "shaders/Ambient.frag");

        mShader_Tonemap = Shader::FromFile("shaders/Tonemap.vert", "shaders/Tonemap.frag");
        mShader_Tonemap.SetUniform("u_sceneTexture", 0);

        mShader_OutlineDilation = Shader::FromFile("shaders/Dilation.vert", "shaders/Dilation.frag");
        mShader_OutlineMask = Shader::FromFile("shaders/Mask.vert", "shaders/Mask.frag");
        mShader_OutlineComposite = Shader::FromFile("shaders/Dilation.vert", "shaders/Outline-Composite.frag");
        mShader_OutlineComposite.SetUniform("u_outlineColor", glm::vec4{ 1.0f, 0.5f, 0.1f, 1.0f });

        mShader_Prefilter = Shader::FromFile("shaders/IBL/Cubemap.vert", "shaders/IBL/Prefilter.frag");
        mShader_Prefilter.SetUniform("u_environmentMap", 0);

        mShader_GenerateIrradianceMap = Shader::FromFile("shaders/IBL/Cubemap.vert", "shaders/IBL/GenerateIrradianceMap.frag");
        mShader_GenerateIrradianceMap.SetUniform("u_environmentMap", 0);

        mShader_GenerateBRDFLUT = Shader::FromFile("shaders/IBL/GenerateBRDFLUT.vert", "shaders/IBL/GenerateBRDFLUT.frag");

        mFBO_OutlineMask = FrameBuffer::CreateMask({ 128, 128 });
        mFBO_OutlineDilation = FrameBuffer::CreateMask({ 128, 128 });


        //// load hdr environment map
        Gep::Texture skyboxTextureEquirectangular = Texture::LoadHDR("assets/textures/HDR/Newport_Loft_Ref.hdr");
        AddTexture(skyboxTextureEquirectangular);
        mEnvironmentCubeMap = EquirectangularToCubemap(skyboxTextureEquirectangular);
        
        //Gep::Texture skyboxIrradianceEquirectangular = Texture::LoadHDR("assets/textures/HDR/Newport_Loft_Ref.irr.hdr");
        //AddTexture(skyboxIrradianceEquirectangular);
        //mIrradianceCubeMap = EquirectangularToCubemap(skyboxIrradianceEquirectangular);

        mIrradianceCubeMap = GenerateIrradianceMap(mEnvironmentCubeMap);
        Texture irradianceEquirectangular = CubemapToEquirectangular(mIrradianceCubeMap);
        AddTexture(irradianceEquirectangular);

        mPrefilterCubeMap = GeneratePrefilterMap(mEnvironmentCubeMap);

        mBRDFLUT = GenerateBRDFLUT();
        AddTexture(mBRDFLUT);

        // gbuffer access in shader
        mShader_PointLight.SetUniform("u_depthTexture", 0);
        mShader_PointLight.SetUniform("u_normalTexture", 1);
        mShader_PointLight.SetUniform("u_colorTexture", 2);
        mShader_PointLight.SetUniform("u_armTexture", 3);

        mShader_PointLightWithShadows.SetUniform("u_depthTexture", 0);
        mShader_PointLightWithShadows.SetUniform("u_normalTexture", 1);
        mShader_PointLightWithShadows.SetUniform("u_colorTexture", 2);
        mShader_PointLightWithShadows.SetUniform("u_armTexture", 3);

        mShader_DirectionalLight.SetUniform("u_depthTexture", 0);
        mShader_DirectionalLight.SetUniform("u_normalTexture", 1);
        mShader_DirectionalLight.SetUniform("u_colorTexture", 2);
        mShader_DirectionalLight.SetUniform("u_armTexture", 3);

        mShader_DirectionalLightWithShadows.SetUniform("u_depthTexture", 0);
        mShader_DirectionalLightWithShadows.SetUniform("u_normalTexture", 1);
        mShader_DirectionalLightWithShadows.SetUniform("u_colorTexture", 2);
        mShader_DirectionalLightWithShadows.SetUniform("u_armTexture", 3);

        mShader_AmbientLight.SetUniform("u_depthTexture", 0);
        mShader_AmbientLight.SetUniform("u_normalTexture", 1);
        mShader_AmbientLight.SetUniform("u_colorTexture", 2);
        mShader_AmbientLight.SetUniform("u_armTexture", 3);
        mShader_AmbientLight.SetUniform("u_brdflut", 4);
        mShader_AmbientLight.SetUniform("u_prefilterMap", 5);
        mShader_AmbientLight.SetUniform("u_irradianceMap", 6);

        SetExposure(1.0f);

        Shader::Unbind();
    }

    uint64_t OpenGLRenderer::AddModel(const Gep::Model& model)
    {
        uint64_t modelIdx = mModelLibrary.emplace();
        auto& entry = mModelLibrary[modelIdx];
        
        for (const Mesh& mesh : model.meshes)
        {
            uint64_t meshIdx = AddMesh(mesh);
            entry.meshes.push_back(meshIdx);
        }

        entry.model = model;
        entry.name = model.name;

        return modelIdx;
    }

    uint64_t OpenGLRenderer::AddMesh(const Gep::Mesh& mesh)
    {
        uint64_t meshIdx = mMeshLibrary.emplace();
        auto& entry = mMeshLibrary[meshIdx];

        entry.handle.GenVertexBuffer(mesh);
        entry.handle.GenIndexBuffer(mesh);
        entry.handle.BindBuffers();

        entry.mesh = mesh;

        return meshIdx;
    }

    uint64_t OpenGLRenderer::AddMaterial(const Gep::Material& material)
    {
        MaterialGPUData gpuData{
            .ao = material.ao,
            .roughness = material.roughness,
            .metalness = material.metalness,
            .color = material.color,

            .aoTextureHandle = material.aoTexture.handle,
            .roughnessTextureHandle = material.roughnessTexture.handle,
            .metalnessTextureHandle = material.metalnessTexture.handle,
            .colorTextureHandle = material.diffuseTexture.handle,
            .normalTextureHandle = material.normalTexture.handle
        };

        MaterialLibraryEntry entry{
            .material = material
        };

        size_t matIdxCPU = mMaterialLibrary.emplace(entry);
        size_t matIdxGPU = mMaterials.emplace(gpuData);

        if (matIdxCPU != matIdxGPU)
            Gep::Log::Critical("CPU and GPU materials are out of sync");

        mMaterials.commit();
        return matIdxCPU;
    }

    uint64_t OpenGLRenderer::AddAnimation(const Gep::Animation& animation)
    {
        uint64_t animIdx = mAnimationLibrary.emplace();
        auto& entry = mAnimationLibrary[animIdx];

        entry.animation = animation;

        return animIdx;
    }

    const Gep::Texture& OpenGLRenderer::GetTexture(uint64_t texIdx) const
    {
        return mTextureLibrary.at(texIdx).texture;
    }

    const Gep::Material& OpenGLRenderer::GetMaterial(uint64_t matIdx) const
    {
        return mMaterialLibrary.at(matIdx).material;
    }

    const Gep::Mesh& OpenGLRenderer::GetMesh(uint64_t meshIdx) const 
    {
        return mMeshLibrary.at(meshIdx).mesh;
    }

    const Gep::Model& OpenGLRenderer::GetModel(uint64_t modelIdx) const
    {
        return mModelLibrary.at(modelIdx).model;
    }

    const std::vector<uint64_t>& OpenGLRenderer::GetModelMeshes(uint64_t modelIdx) const
    {
        return mModelLibrary.at(modelIdx).meshes;
    }

    const Gep::Animation& OpenGLRenderer::GetAnimation(uint64_t animIdx) const
    {
        return mAnimationLibrary.at(animIdx).animation;
    }

    bool OpenGLRenderer::IsTextureLoaded(uint64_t texIdx)
    {
        return mTextureLibrary.contains(texIdx);
    }

    bool OpenGLRenderer::IsMaterialLoaded(uint64_t matIdx)
    {
        return mMaterialLibrary.contains(matIdx);
    }

    bool OpenGLRenderer::IsMeshLoaded(uint64_t meshIdx)
    {
        return mMeshLibrary.contains(meshIdx);
    }

    bool OpenGLRenderer::IsModelLoaded(uint64_t modelIdx)
    {
        return mModelLibrary.contains(modelIdx);
    }

    bool OpenGLRenderer::IsAnimationLoaded(uint64_t animIdx)
    {
        return mAnimationLibrary.contains(animIdx);
    }

    std::optional<uint64_t> OpenGLRenderer::FindTexture(const std::string& texName)
    {
        Gep::Log::Critical("Not Implemented");
        return 0;
    }

    std::optional<uint64_t> OpenGLRenderer::FindMaterial(const std::string& matName)
    {
        Gep::Log::Critical("Not Implemented");
        return 0;
    }

    std::optional<uint64_t> OpenGLRenderer::FindMesh(const std::string& meshName)
    {
        Gep::Log::Critical("Not Implemented");
        return 0;
    }

    std::optional<uint64_t> OpenGLRenderer::FindModel(const std::string& modelName)
    {
        auto it = std::find_if(mModelLibrary.begin(), mModelLibrary.end(), [&](auto pair) 
        {
            auto& [modelIdx, entry] = pair;

            return (entry.model.name == modelName);
        });

        if (it == mModelLibrary.end())
            return std::nullopt;

        return (*it).first;
    }

    std::optional<uint64_t> OpenGLRenderer::FindAnimation(const std::string& animName)
    {
        auto it = std::find_if(mAnimationLibrary.begin(), mAnimationLibrary.end(), [&](auto pair)
        {
            auto& [modelIdx, entry] = pair;

            return (entry.animation.name == animName);
        });

        if (it == mAnimationLibrary.end())
            return std::nullopt;

        return (*it).first;
    }

    uint64_t OpenGLRenderer::AddTexture(const Gep::Texture& texture)
    {
        uint64_t texIdx = mTextureLibrary.emplace();
        auto& entry = mTextureLibrary[texIdx];

        entry.texture = texture;

        return texIdx;
    }

    void OpenGLRenderer::AddObject(const AddObjectInfo& drawInfo)
    {
        if (!IsModelLoaded(drawInfo.modelIdx))
        {
            Gep::Log::Error("Failed to draw object. The model: [", drawInfo, "] doesn't exist");
            return;
        }

        RenderFlags flags = RenderFlags::None;
        ShaderType type = (drawInfo.boneOffset == Gep::NumMax<uint32_t>()) ? ShaderType::Static : ShaderType::Rigged;

        if (drawInfo.outline.has_value())
            flags |= RenderFlags::Highlight;

        if (drawInfo.wireframe.has_value())
            flags |= RenderFlags::Wireframe;

        mObjectDatas[type][drawInfo.modelIdx][flags].push_back(drawInfo);
    }

    //void OpenGLRenderer::AddObject(uint64_t modelIdx, const ObjectInstanceDataGPU& gpuData, RenderFlags flags)
    //{
    //    if (!IsModelLoaded(modelIdx))
    //    {
    //        Gep::Log::Error("Failed to draw object. The model: [", modelIdx, "] doesn't exist");
    //        return;
    //    }

    //    mObjectDatas[modelIdx][flags].push_back(gpuData);
    //}

    void OpenGLRenderer::AddCamera(const CameraGPUData& uniforms)
    {
        mCameraUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddPointLight(const PointLightGPUData& uniforms)
    {
        mPointLightUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddPointLightShadow(const PointLightShadowGPUData& uniforms, const FrameBuffer& fbo)
    {
        mPointLightShadowUniforms.push_back(uniforms);
        mPointLightShadowMaps.push_back(fbo);
    }

    void OpenGLRenderer::AddDirectionalLight(const DirectionalLightGPUData& uniforms)
    {
        mDirectionalLightUniforms.push_back(uniforms);
    }

    void OpenGLRenderer::AddDirectionalLightShadow(const DirectionalLightShadowGPUData& uniforms, const FrameBuffer& fbo)
    {
        mDirectionalLightShadowUniforms.push_back(uniforms);
        mDirectionalLightShadowMaps.push_back(fbo);
    }

    void OpenGLRenderer::AddBone(const BoneGPUData& boneData)
    {
        mBoneUniforms.push_back(boneData);
    }

    void OpenGLRenderer::AddLine(const LineGPUData& lines)
    {
        mLineUniforms.push_back(lines);
    }

    void OpenGLRenderer::CommitObjects()
    {
        for (const auto& [shaderType, models] : mObjectDatas)
        for (const auto& [modelIdx, flagsToObjects] : models)
        {
            const auto& modelEntry = mModelLibrary.at(modelIdx);

            for (const auto& [flags, objects] : flagsToObjects)
            {
                // add all per object instance data in instance order
                for (const auto& object : objects)
                {
                    ObjectInstanceDataGPU gpuData{
                        .modelMatrix = object.modelMatrix,
                        .normalMatrixCol0 = object.normalMatrix[0],
                        .normalMatrixCol1 = object.normalMatrix[1],
                        .normalMatrixCol2 = object.normalMatrix[2],
                        .boneOffset = Gep::NumMax<int>()
                    };

                    mStaticObjectUniforms.push_back(gpuData);
                }

                // add per mesh instance data, ordering memory like [0][0][0][0][1][1][1][1][2][2][2][2]
                for (uint32_t localMeshIdx = 0; localMeshIdx < modelEntry.meshes.size(); ++localMeshIdx)
                {
                    uint32_t meshIdx = modelEntry.meshes[localMeshIdx];
                    auto& meshEntry = mMeshLibrary[meshIdx];

                    // per mesh then per instance
                    for (uint32_t instanceIdx = 0; instanceIdx < objects.size(); ++instanceIdx)
                    {
                        const auto& object = objects[instanceIdx];

                        uint32_t matIdx = meshEntry.mesh.materialIndex;
                        if (localMeshIdx < object.materialIdxs.size())
                            matIdx = object.materialIdxs[localMeshIdx];

                        MeshGPUData meshData{ .materialIndex = matIdx };
                        mMeshUniforms.push_back(meshData);
                    }
                }
            }
        }

        // copies instance information to the gpu
        mStaticObjectUniforms.commit();
        mMeshUniforms.commit();
    }

    void OpenGLRenderer::CommitCameras()
    {
        mCameraUniforms.commit();
    }

    void OpenGLRenderer::CommitBones()
    {
        mBoneUniforms.commit();
    }

    void OpenGLRenderer::CommitLights()
    {
        mPointLightUniforms.commit();
        mPointLightShadowUniforms.commit();
        mDirectionalLightUniforms.commit();
        mDirectionalLightShadowUniforms.commit();
    }

    void OpenGLRenderer::SetCameraIndex(uint32_t index)
    {
        std::apply([index](auto&... shaders) 
        {
            ((shaders.SetUniform(0, index)), ...);
        }, 
        GetAllShaders());
    }

    void OpenGLRenderer::UnloadTexture(uint64_t texIdx)
    {
        auto& entry = mTextureLibrary[texIdx];

        // 1. Make handle non-resident (required for bindless textures)
        if (entry.texture.handle)
        {
            glMakeTextureHandleNonResidentARB(entry.texture.handle);
            entry.texture.handle = 0;
        }

        // 2. Delete the texture object
        if (entry.texture.id)
        {
            glDeleteTextures(1, &entry.texture.id);
            entry.texture.id = 0;
        }

        mTextureLibrary.erase(texIdx);
    }

    void OpenGLRenderer::UnloadMaterial(uint64_t matIdx)
    {
        mMaterialLibrary.erase(matIdx);
        mMaterials.erase(matIdx);
    }

    void OpenGLRenderer::UnloadModel(uint64_t modelIdx)
    {
        if (!IsModelLoaded(modelIdx))
        {
            Gep::Log::Error("Cannot unload mesh: [", modelIdx, "] a mesh with that name has not been loaded");
            return;
        }

        // aquire the model id from the name
        auto& entry = mModelLibrary[modelIdx];

        // delete all meshes owned by the model
        for (uint64_t meshIdx : entry.meshes)
        {
            UnloadMesh(meshIdx);
        }

        for (auto& [shaderType, models] : mObjectDatas)
        {
            models.erase(modelIdx);
        }
        mModelLibrary.erase(modelIdx);
    }

    void OpenGLRenderer::UnloadMesh(uint64_t meshIdx)
    {
        if (!IsMeshLoaded(meshIdx))
        {
            Gep::Log::Error("Cannot unload mesh: [", meshIdx, "] a mesh with that name has not been loaded");
            return;
        }

        auto& entry = mMeshLibrary[meshIdx];

        entry.handle.DeleteBuffers();

        mMeshLibrary.erase(meshIdx);
    }

    void OpenGLRenderer::UnloadAnimation(uint64_t animIdx)
    {
        mAnimationLibrary.erase(animIdx);
    }

    void OpenGLRenderer::Start(const glm::vec3& color)
    {
        //glClearColor(color.r, color.g, color.b, 1);
        //glClearDepth(1);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    std::vector<std::string> OpenGLRenderer::GetLoadedModelNames() const
    {
        std::vector<std::string> modelNames;
        modelNames.reserve(mModelLibrary.size());

        for (const auto [idx, entry] : mModelLibrary)
            modelNames.emplace_back(entry.name);

        return modelNames;
    }

    std::vector<std::string> OpenGLRenderer::GetLoadedAnimationNames() const
    {
        std::vector<std::string> animNames;
        animNames.reserve(mAnimationLibrary.size());

        for (const auto [idx, entry] : mAnimationLibrary)
            animNames.emplace_back(entry.animation.name);

        return animNames;
    }

    std::vector<Texture> OpenGLRenderer::GetLoadedTextures() const
    {
        std::vector<Texture> textures;
        textures.reserve(mTextureLibrary.size());

        for (const auto [idx, entry] : mTextureLibrary)
            textures.emplace_back(entry.texture);

        return textures;
    }

    const std::vector<std::string>& OpenGLRenderer::GetSupportedModelFormats() const
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

    const std::vector<std::string>& OpenGLRenderer::GetSupportedTextureFormats() const
    {
        static std::vector<std::string> allowedExtensions = { ".jpg", ".jpeg", ".png", ".bmp" };

        return allowedExtensions;
    }

    //Texture OpenGLRenderer::GetOrLoadIconTexture(const std::filesystem::path& iconPath)
    //{
    //    if (!mIconTextures.contains(iconPath.extension().string()))
    //        LoadIconTexture(iconPath);

    //    // check if the icon path is a supported image format
    //    const std::vector<std::string>& supportedTextureFormats = GetSupportedTextureFormats();
    //    auto it = std::find(supportedTextureFormats.begin(), supportedTextureFormats.end(), iconPath.extension().string());

    //    // if the icon is a supported image, use the image itself
    //    if (it != supportedTextureFormats.end())
    //    {
    //        return GetOrLoadTexture(iconPath);
    //    }

    //    return mIconTextures.at(iconPath.extension().string());
    //}

    void OpenGLRenderer::ReloadShaders()
    {
        std::apply([](auto&... shaders)
        {
            ((shaders.Reload()), ...);
        }, 
        GetAllShaders());

        mShader_EquirectangularToCubemap.SetUniform("u_equirectangularMap", 0);
        mShader_CubemapToEquirectangular.SetUniform("u_cubeMap", 0);
        mShader_Background.SetUniform("u_environmentMap", 0);

        mShader_Tonemap.SetUniform("u_sceneTexture", 0);
        SetExposure(1.0f);

        const uint32_t outlineSize = 2;
        mShader_OutlineDilation.SetUniform("u_direction", glm::vec2{ 0,1 });
        mShader_OutlineDilation.SetUniform("u_radius", outlineSize);

        mShader_OutlineDilationVertical.SetUniform("u_direction", glm::vec2{ 1,0 });
        mShader_OutlineDilationVertical.SetUniform("u_radius", outlineSize);

        mShader_OutlineComposite.SetUniform("u_outlineColor", glm::vec4{ 1.0f, 0.1f, 0.1f, 1.0f });

        // gbuffer access in shader
        mShader_PointLight.SetUniform("u_depthTexture", 0);
        mShader_PointLight.SetUniform("u_normalTexture", 1);
        mShader_PointLight.SetUniform("u_colorTexture", 2);
        mShader_PointLight.SetUniform("u_armTexture", 3);

        mShader_PointLightWithShadows.SetUniform("u_depthTexture", 0);
        mShader_PointLightWithShadows.SetUniform("u_normalTexture", 1);
        mShader_PointLightWithShadows.SetUniform("u_colorTexture", 2);
        mShader_PointLightWithShadows.SetUniform("u_armTexture", 3);

        mShader_DirectionalLight.SetUniform("u_depthTexture", 0);
        mShader_DirectionalLight.SetUniform("u_normalTexture", 1);
        mShader_DirectionalLight.SetUniform("u_colorTexture", 2);
        mShader_DirectionalLight.SetUniform("u_armTexture", 3);

        mShader_DirectionalLightWithShadows.SetUniform("u_depthTexture", 0);
        mShader_DirectionalLightWithShadows.SetUniform("u_normalTexture", 1);
        mShader_DirectionalLightWithShadows.SetUniform("u_colorTexture", 2);
        mShader_DirectionalLightWithShadows.SetUniform("u_armTexture", 3);

        mShader_AmbientLight.SetUniform("u_depthTexture", 0);
        mShader_AmbientLight.SetUniform("u_normalTexture", 1);
        mShader_AmbientLight.SetUniform("u_colorTexture", 2);
        mShader_AmbientLight.SetUniform("u_armTexture", 3);
        mShader_AmbientLight.SetUniform("u_brdflut", 4);
        mShader_AmbientLight.SetUniform("u_prefilterMap", 5);
        mShader_AmbientLight.SetUniform("u_irradianceMap", 6);
    }

    void OpenGLRenderer::SetExposure(float exposure)
    {
        mShader_Tonemap.SetUniform(0, exposure);
    }

    void OpenGLRenderer::Draw(Gep::FrameBuffer& targetFrameBuffer)
    {
        static FrameBuffer hdrSceneFrameBuffer = FrameBuffer::CreateScreenHDR(targetFrameBuffer.GetSize());
        hdrSceneFrameBuffer.Bind();
        hdrSceneFrameBuffer.Resize(targetFrameBuffer.GetSize()); // make sure the gbuffer is the same size as the target framebuffer
        hdrSceneFrameBuffer.UpdateViewport();
        hdrSceneFrameBuffer.Clear();
        FrameBuffer::Unbind();

        // render to depth cube buffer here
        PointLightShadowDepthPass();            // renders all scene geometry for each point light that casts shadows to the corresponding shadow map
        DirectionalLightShadowDepthPass();
        DrawLines(hdrSceneFrameBuffer);
        GeometryPass(hdrSceneFrameBuffer);   // renders all scene geometry to the gbuffer
        DirectionalLightPass(hdrSceneFrameBuffer);
        PointLightPass(hdrSceneFrameBuffer); // renders all point lights as light volumes, using the gbuffer for shading
        AmbientPass(hdrSceneFrameBuffer);
        BackgroundPass(hdrSceneFrameBuffer, mEnvironmentCubeMap);
        TonemapPass(targetFrameBuffer, hdrSceneFrameBuffer);

        // draw postprocess effects, ie model outlines
        OutlinePass(targetFrameBuffer);
    }

    void OpenGLRenderer::End()
    {
        mLineUniforms.clear();

        for (auto& [modelName, flagsToObjects] : mObjectDatas)
        {
            for (auto& [flags, objects] : flagsToObjects)
            {
                objects.clear();
            }
        }

        mPointLightShadowMaps.clear();
        mPointLightShadowUniforms.clear();
        mPointLightUniforms.clear();

        mDirectionalLightShadowMaps.clear();
        mDirectionalLightUniforms.clear();
        mDirectionalLightShadowUniforms.clear();

        mStaticObjectUniforms.clear();
        mCameraUniforms.clear();
        mBoneUniforms.clear();
        mMeshUniforms.clear();

        mStats.drawCalls = 0;
        mStats.vertexCount = 0;
    }

    void OpenGLRenderer::SetUpLineDrawing()
    {
        glGenVertexArrays(1, &mLineVAO);
        glGenBuffers(1, &mLineVBO);

        glBindVertexArray(mLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mLineVBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void OpenGLRenderer::GeometryPass(const Gep::FrameBuffer& targetFrameBuffer)
    {
        mGeometryFrameBuffer.Bind();
        mGeometryFrameBuffer.Resize(targetFrameBuffer.GetSize()); // make sure the gbuffer is the same size as the target framebuffer
        mGeometryFrameBuffer.UpdateViewport();
        mGeometryFrameBuffer.Clear();
        mGeometryFrameBuffer.DrawBuffers();

        GLDrawFlags flags{
            .depthFuncMask = std::make_pair(GL_LEQUAL, GL_TRUE),
            .cullMode = GL_BACK, // back face culling
            .blendFuncSD = std::nullopt // no blending
        };
        SetDrawFlags(flags);

        uint32_t baseInstance = 0;
        uint32_t meshBaseInstance = 0;

        mShader_Geometry.Bind();

        for (const auto& [shaderType, models] : mObjectDatas)
        {
            if (shaderType == ShaderType::Rigged)
                mShader_Geometry.SetUniform(5, true); // enable rigged
            else //if (shaderType == ShaderType::Static)
                mShader_Geometry.SetUniform(5, false);

            for (const auto& [modelIdx, flagsToObjects] : models)
            {
                const auto& modelEntry = mModelLibrary.at(modelIdx);

                for (const auto& [flags, objects] : flagsToObjects)
                {
                    for (uint64_t meshIdx : modelEntry.meshes)
                    {
                        const auto& meshHandle = mMeshLibrary[meshIdx].handle;

                        mShader_Geometry.SetUniform(3, meshBaseInstance);

                        GLDraw(meshHandle.mVertexArrayObject, meshHandle.mIndexCount, objects.size(), baseInstance);

                        meshBaseInstance += objects.size();
                    }
                    baseInstance += objects.size();
                }
            }
        }

        Shader::Unbind();
        FrameBuffer::Unbind();
    }

    void OpenGLRenderer::PointLightPass(Gep::FrameBuffer& targetFrameBuffer)
    {
        targetFrameBuffer.Bind(); // draw to the target framebuffer
        mGeometryFrameBuffer.BindTextures(); // bind gbuffer textures to texture units

        GLDrawFlags flags{
            .depthFuncMask = std::nullopt, // do not use depth
            .cullMode = GL_FRONT, // front face culling
            .blendFuncSD = std::make_pair(GL_ONE, GL_ONE) // one one blending
        };
        SetDrawFlags(flags);

        auto& sphereHandle = mMeshLibrary[mSphereMeshIndex].handle;

        mShader_PointLight.Bind(); // draw pass for lights that do not cast shadows
        GLDraw(sphereHandle.mVertexArrayObject, sphereHandle.mIndexCount, mPointLightUniforms.size(), 0);

        mShader_PointLightWithShadows.Bind(); // draw pass for lights that cast shadows
        GLDraw(sphereHandle.mVertexArrayObject, sphereHandle.mIndexCount, mPointLightShadowUniforms.size(), 0);

        Shader::Unbind();
    }

    void OpenGLRenderer::PointLightShadowDepthPass()
    {
        GLDrawFlags flags{
            .depthFuncMask = std::make_pair(GL_LEQUAL, GL_TRUE),
            .cullMode = GL_BACK, // back face culling
            .blendFuncSD = std::nullopt // no blending
        };
        SetDrawFlags(flags);

        uint32_t lightIndex = 0;
        mShader_PointLightShadowDepth.Bind();
        for (const FrameBuffer& shadowMap : mPointLightShadowMaps)
        {
            shadowMap.Bind();
            shadowMap.UpdateViewport();
            glClear(GL_DEPTH_BUFFER_BIT);

            uint32_t baseInstance = 0;
            mShader_PointLightShadowDepth.SetUniform(2, lightIndex++);

            for (const auto& [shaderType, models] : mObjectDatas)
            {
                if (shaderType == ShaderType::Rigged)
                    mShader_PointLightShadowDepth.SetUniform(5, true); // enable rigged
                else //if (shaderType == ShaderType::Static)
                    mShader_PointLightShadowDepth.SetUniform(5, false);

                for (const auto& [modelIdx, flagsToObjects] : models)
                {
                    const auto& modelEntry = mModelLibrary.at(modelIdx);

                    for (const auto& [flags, objects] : flagsToObjects)
                    {
                        for (uint64_t meshIdx : modelEntry.meshes)
                        {
                            const auto& meshHandle = mMeshLibrary[meshIdx].handle;

                            GLDraw(meshHandle.mVertexArrayObject, meshHandle.mIndexCount, objects.size(), baseInstance);
                        }
                        baseInstance += objects.size();
                    }
                }
            }
        }

        Shader::Unbind();
        FrameBuffer::Unbind();
    }

    void OpenGLRenderer::DirectionalLightPass(Gep::FrameBuffer& targetFrameBuffer)
    {
        targetFrameBuffer.Bind();          // draw to the target framebuffer
        mGeometryFrameBuffer.BindTextures(); // bind gbuffer textures to texture units

        GLDrawFlags flags{
            .depthFuncMask = std::nullopt,
            .cullMode = GL_BACK, // back face culling
            .blendFuncSD = std::make_pair(GL_ONE, GL_ONE)
        };
        SetDrawFlags(flags);

        mShader_DirectionalLight.Bind();
        mStats.drawCalls++;
        glDrawArraysInstanced(
            GL_TRIANGLES,
            0,
            3,
            static_cast<GLsizei>(mDirectionalLightUniforms.size())
        );

        mShader_DirectionalLightWithShadows.Bind();
        mStats.drawCalls++;
        glDrawArraysInstanced(
            GL_TRIANGLES,
            0,
            3,
            static_cast<GLsizei>(mDirectionalLightShadowUniforms.size())
        );

        Shader::Unbind();
    }

    void OpenGLRenderer::DirectionalLightShadowDepthPass()
    {
        GLDrawFlags flags{
            .depthFuncMask = std::make_pair(GL_LEQUAL, GL_TRUE),
            .cullMode = GL_BACK, // back face culling
            .blendFuncSD = std::nullopt
        };
        SetDrawFlags(flags);

        uint32_t lightIndex = 0;
        mShader_DirectionalLightShadowDepth.Bind();
        for (const FrameBuffer& shadowMap : mDirectionalLightShadowMaps)
        {
            shadowMap.Bind();
            shadowMap.UpdateViewport();
            glClear(GL_DEPTH_BUFFER_BIT);

            uint32_t baseInstance = 0;
            mShader_DirectionalLightShadowDepth.SetUniform(2, lightIndex++);

            for (const auto& [shaderType, models] : mObjectDatas)
            {
                if (shaderType == ShaderType::Rigged)
                    mShader_DirectionalLightShadowDepth.SetUniform(5, true); // enable rigged
                else //if (shaderType == ShaderType::Static)
                    mShader_DirectionalLightShadowDepth.SetUniform(5, false);

                for (const auto& [modelIdx, flagsToObjects] : models)
                {
                    const auto& modelEntry = mModelLibrary.at(modelIdx);

                    for (const auto& [flags, objects] : flagsToObjects)
                    {
                        for (uint64_t meshIdx : modelEntry.meshes)
                        {
                            const auto& meshHandle = mMeshLibrary[meshIdx].handle;

                            GLDraw(meshHandle.mVertexArrayObject, meshHandle.mIndexCount, objects.size(), baseInstance);
                        }
                        baseInstance += objects.size();
                    }
                }
            }
        }

        Shader::Unbind();
        FrameBuffer::Unbind();
    }

    void OpenGLRenderer::DrawLines(Gep::FrameBuffer& targetFrameBuffer)
    {
        GLDrawFlags flags{
            .depthFuncMask = std::nullopt,
            .cullMode = std::nullopt,
            .blendFuncSD = std::nullopt
        };
        SetDrawFlags(flags);

        targetFrameBuffer.Bind();
        mShader_Line.Bind();

        glBindVertexArray(mLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mLineVBO);

        for (const LineGPUData& lineData : mLineUniforms)
        {
            // one color per set
            mShader_Line.SetUniform(1, glm::vec4(lineData.color, 1.0f));

            glBufferData(GL_ARRAY_BUFFER,
                lineData.points.size() * sizeof(glm::vec3) * 2,
                lineData.points.data(),
                GL_STREAM_DRAW
            );

            // draw all line segments in this set
            mStats.drawCalls++;
            glDrawArrays(GL_LINES, 0, lineData.points.size() * 2);
        }

        mShader_Line.Unbind();
    }

    void OpenGLRenderer::BackgroundPass(Gep::FrameBuffer& targetFrameBuffer, const Gep::Texture& backgroundCubeMap)
    {
        const glm::ivec2 targetSize = targetFrameBuffer.GetSize();

        // copies the depth buffer from the geometry buffer to the target frame buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mGeometryFrameBuffer.GetFrameBufferID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFrameBuffer.GetFrameBufferID());
        glBlitFramebuffer(0, 0, targetSize.x, targetSize.y, 0, 0, targetSize.x, targetSize.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        GLDrawFlags flags{
            .depthFuncMask = std::make_pair(GL_LEQUAL, GL_FALSE),
            .cullMode = std::nullopt,
            .blendFuncSD = std::nullopt
        };

        targetFrameBuffer.Bind();

        mShader_Background.Bind();
        mShader_Background.SetTextureCube(0, backgroundCubeMap.id);

        // draw cube
        auto& cubeHandle = mMeshLibrary[mCubeMeshIndex].handle;

        SetDrawFlags(flags);
        GLDraw(cubeHandle.mVertexArrayObject, cubeHandle.mIndexCount, 1, 0);
    }

    void OpenGLRenderer::AmbientPass(Gep::FrameBuffer& targetFrameBuffer)
    {
        GLDrawFlags flags{
            .depthFuncMask = std::nullopt,
            .cullMode = std::nullopt,
            .blendFuncSD = std::make_pair(GL_ONE, GL_ONE)
        };

        targetFrameBuffer.Bind();
        mGeometryFrameBuffer.BindTextures(); // bind gbuffer textures to texture units

        mShader_AmbientLight.Bind();
        mShader_AmbientLight.SetTexture2D  (mGeometryFrameBuffer.GetTextureCount() + 0, mBRDFLUT.id);
        mShader_AmbientLight.SetTextureCube(mGeometryFrameBuffer.GetTextureCount() + 1, mPrefilterCubeMap.id);
        mShader_AmbientLight.SetTextureCube(mGeometryFrameBuffer.GetTextureCount() + 2, mIrradianceCubeMap.id);

        SetDrawFlags(flags);
        GLDrawQuad();
        Shader::Unbind();
    }

    void OpenGLRenderer::TonemapPass(Gep::FrameBuffer& ldrFrameBuffer, const Gep::FrameBuffer& hdrFrameBuffer)
    {
        GLDrawFlags flags{
            .depthFuncMask = std::nullopt,
            .cullMode = std::nullopt,
            .blendFuncSD = std::nullopt
        };

        // tone map pass
        ldrFrameBuffer.Bind();
        mShader_Tonemap.Bind();
        mShader_Tonemap.SetTexture2D(0, hdrFrameBuffer.GetTexture(0));

        SetDrawFlags(flags);
        GLDrawQuad();
        Shader::Unbind();
    }

    void OpenGLRenderer::OutlinePass(Gep::FrameBuffer& targetFrameBuffer)
    {
        GLDrawFlags sceneMaskFlags{
            .depthFuncMask = std::nullopt,
            .cullMode = std::nullopt,
            .blendFuncSD = std::nullopt
        };

        GLDrawFlags postProcessFlags{
            .depthFuncMask = std::nullopt,
            .cullMode = std::nullopt,
            .blendFuncSD = std::nullopt
        };

        GLDrawFlags compositeFlags{
            .depthFuncMask = std::nullopt,
            .cullMode = std::nullopt,
            .blendFuncSD = std::make_pair(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        };

        // setup outline mask fb
        mFBO_OutlineMask.Bind();
        mFBO_OutlineMask.Resize(targetFrameBuffer.GetSize()); // make sure the gbuffer is the same size as the target framebuffer
        mFBO_OutlineMask.UpdateViewport();
        mFBO_OutlineMask.Clear();

        // setup dilation fb
        mFBO_OutlineDilation.Bind();
        mFBO_OutlineDilation.Resize(targetFrameBuffer.GetSize()); // make sure the gbuffer is the same size as the target framebuffer
        mFBO_OutlineDilation.UpdateViewport();
        mFBO_OutlineDilation.Clear();

        // draw the object mask ///////////////////////////////////////////////////////////
        mFBO_OutlineMask.Bind();
        mShader_OutlineMask.Bind();
        mShader_OutlineMask.SetUniform("u_color", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});

        SetDrawFlags(sceneMaskFlags);
        
        uint32_t baseInstance = 0;
        for (const auto& [shaderType, models] : mObjectDatas)
        {
            if (shaderType == ShaderType::Rigged)
                mShader_OutlineMask.SetUniform(5, true); // enable rigged
            else //if (shaderType == ShaderType::Static)
                mShader_OutlineMask.SetUniform(5, false);

            for (const auto& [modelIdx, flagsToObjects] : models)
            {
                const auto& modelEntry = mModelLibrary.at(modelIdx);

                for (const auto& [flags, objects] : flagsToObjects)
                {
                    bool hasOutLine = (flags & RenderFlags::Highlight) == RenderFlags::Highlight;

                    if (hasOutLine)
                    for (uint64_t meshIdx : modelEntry.meshes)
                    {
                        const auto& meshHandle = mMeshLibrary[meshIdx].handle;

                        GLDraw(meshHandle.mVertexArrayObject, meshHandle.mIndexCount, objects.size(), baseInstance);
                    }
                    baseInstance += objects.size();
                }
            }
        }
        //// draw the object dialated horizontal //////////////////////////////////////////
        mFBO_OutlineDilation.Bind();
        mShader_OutlineDilation.Bind();
        mShader_OutlineDilation.SetTexture2D(0, mFBO_OutlineMask.GetTexture(0));
        mShader_OutlineDilation.SetUniform("u_direction", glm::vec2{ 0,1 });
        mShader_OutlineDilation.SetUniform("u_radius", 2);

        SetDrawFlags(postProcessFlags);
        GLDrawQuad();

        // draw the object dialated vertical //////////////////////////////////////////////////
        mFBO_OutlineMask.Bind();
        mShader_OutlineDilation.Bind();
        mShader_OutlineDilation.SetTexture2D(0, mFBO_OutlineDilation.GetTexture(0));
        mShader_OutlineDilation.SetUniform("u_direction", glm::vec2{ 1,0 });
        mShader_OutlineDilation.SetUniform("u_radius", 2);

        SetDrawFlags(postProcessFlags);
        GLDrawQuad();

        // draw the object again but inverted so it cookie cutters ///////////////////////////////
        mFBO_OutlineMask.Bind();
        mShader_OutlineMask.Bind();
        mShader_OutlineMask.SetUniform("u_color", glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f });

        SetDrawFlags(sceneMaskFlags);
        baseInstance = 0;
        for (const auto& [shaderType, models] : mObjectDatas)
        {
            if (shaderType == ShaderType::Rigged)
                mShader_OutlineMask.SetUniform(5, true); // enable rigged
            else //if (shaderType == ShaderType::Static)
                mShader_OutlineMask.SetUniform(5, false);

            for (const auto& [modelIdx, flagsToObjects] : models)
            {
                const auto& modelEntry = mModelLibrary.at(modelIdx);

                for (const auto& [flags, objects] : flagsToObjects)
                {
                    bool hasOutLine = (flags & RenderFlags::Highlight) == RenderFlags::Highlight;

                    if (hasOutLine)
                    for (uint64_t meshIdx : modelEntry.meshes)
                    {
                        const auto& meshHandle = mMeshLibrary[meshIdx].handle;

                        GLDraw(meshHandle.mVertexArrayObject, meshHandle.mIndexCount, objects.size(), baseInstance);
                    }
                    baseInstance += objects.size();
                }
            }
        }

        // composite the outline mask over the final scene /////////////////////////////////////////////
        targetFrameBuffer.Bind();
        mShader_OutlineComposite.Bind();
        mShader_OutlineComposite.SetTexture2D(0, mFBO_OutlineMask.GetTexture(0));

        SetDrawFlags(compositeFlags);
        GLDrawQuad();

        Shader::Unbind();
        FrameBuffer::Unbind();
    }

    void OpenGLRenderer::MeshGPUHandle::GenVertexBuffer(const Mesh& mesh)
    {
        if (mesh.vertices.empty())
        {
            Gep::Log::Error("Cannot gen vertex buffer of size 0");
            return;
        }

        glGenBuffers(1, &mVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_STATIC_DRAW);
    }

    void OpenGLRenderer::MeshGPUHandle::GenIndexBuffer(const Mesh& mesh)
    {
        if (mesh.indices.empty())
        {
            Gep::Log::Error("Cannot gen index buffer of size 0");
            return;
        }

        glGenBuffers(1, &mIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW);

        mIndexCount = mesh.indices.size();
    }

    void OpenGLRenderer::MeshGPUHandle::BindBuffers()
    {
        if (mVertexBuffer == NULL || mIndexBuffer == NULL)
        {
            Gep::Log::Error("Cannot bind buffers on invalid buffers");
            return;
        }

        glGenVertexArrays(1, &mVertexArrayObject);
        glBindVertexArray(mVertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIndices));
        glEnableVertexAttribArray(3);

        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
        glEnableVertexAttribArray(4);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

        glBindVertexArray(0);
    }

    void OpenGLRenderer::MeshGPUHandle::DeleteBuffers()
    {
        glDeleteBuffers(1, &mIndexBuffer);
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteVertexArrays(1, &mVertexArrayObject);

#ifdef _DEBUG
        mVertexArrayObject = NumMax<GLuint>();
        mVertexBuffer = NumMax<GLuint>();
        mIndexBuffer = NumMax<GLuint>();
#endif // _DEBUG
    }

    struct BoneInfo
    {
        uint32_t index = 0;
        Gep::VQS offset{};
    };

    static std::unordered_map<std::string, BoneInfo> gBoneData;

    Texture OpenGLRenderer::LoadTexturesFromAssimpMaterial(const std::filesystem::path& modelPath, const aiMaterial* assimpMaterial, const aiScene* scene, const aiTextureType type)
    {
        auto root = modelPath.parent_path();

        aiString texPath;
        if (aiReturn_SUCCESS != assimpMaterial->GetTexture(type, 0, &texPath))
            return {}; // this material does not contain a texture of the given type

        if (texPath.C_Str()[0] == '*') // if the first character is a star it is embedded
        {
            int assimpTextureIndex = std::atoi(texPath.C_Str() + 1);
            aiTexture* assimpTexture = scene->mTextures[assimpTextureIndex];
            std::string textureName = modelPath.string() + "_EMBEDDED_" + std::to_string(assimpTextureIndex);

            if (assimpTexture->mHeight == 0) // if no height then it is compressed
            {
                std::vector<uint8_t> bytes(
                    reinterpret_cast<uint8_t*>(assimpTexture->pcData),
                    reinterpret_cast<uint8_t*>(assimpTexture->pcData) + assimpTexture->mWidth
                );

                return Texture::LoadFromMemory(bytes.data(), bytes.size());
            }

            const int assimpTextureChannels = 4;
            // BGRA? format may cause issues remember this
            Gep::Log::Critical("I'm not sure if this is ever used so this is going to crash if this is");
            return Texture::LoadFromPixels(reinterpret_cast<uint8_t*>(assimpTexture->pcData), assimpTexture->mWidth, assimpTexture->mHeight, assimpTextureChannels);
        }
        
        return Texture::Load(root / texPath.C_Str());
    }

    void OpenGLRenderer::LoadAnimation(const std::string& parentPath, const aiAnimation* assimpAnimation, const Skeleton& skeleton)
    {
        uint64_t animIdx = mAnimationLibrary.emplace();
        auto& entry = mAnimationLibrary[animIdx];
        auto& animation = entry.animation;

        animation.duration = static_cast<float>(assimpAnimation->mDuration);
        animation.ticksPerSecond = assimpAnimation->mTicksPerSecond != 0.0
            ? static_cast<float>(assimpAnimation->mTicksPerSecond)
            : 25.0f; // Assimp default

        animation.tracks.reserve(assimpAnimation->mNumChannels);

        for (uint32_t i = 0; i < assimpAnimation->mNumChannels; i++)
        {
            const aiNodeAnim* channel = assimpAnimation->mChannels[i];

            // find bone index in skeleton
            auto it = std::find_if(skeleton.bones.begin(), skeleton.bones.end(), [&](const Bone& b)
            {
                return b.name == channel->mNodeName.C_Str();
            });

            if (it == skeleton.bones.end())
            {
                Gep::Log::Warning("Animation channel for bone '", channel->mNodeName.C_Str(), "' not found in skeleton");
                continue;
            }

            uint32_t boneIndex = static_cast<uint32_t>(std::distance(skeleton.bones.begin(), it));

            Track& track = animation.tracks.emplace_back();
            track.boneIndex = boneIndex;

            // loop through all keyframes reading in their time and position
            track.positionKeyFrames.reserve(channel->mNumPositionKeys);
            track.rotationKeyFrames.reserve(channel->mNumRotationKeys);
            track.scaleKeyFrames.reserve(channel->mNumScalingKeys);

            for (size_t k = 0; k < channel->mNumPositionKeys; k++)
            {
                auto& keyFrame = track.positionKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mPositionKeys[k].mTime);
                keyFrame.transform = ToVec3(channel->mPositionKeys[k].mValue);
            }
            for (size_t k = 0; k < channel->mNumRotationKeys; k++)
            {
                auto& keyFrame = track.rotationKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mRotationKeys[k].mTime);
                keyFrame.transform = glm::normalize(ToQuat(channel->mRotationKeys[k].mValue));
            }
            for (size_t k = 0; k < channel->mNumScalingKeys; k++)
            {
                auto& keyFrame = track.scaleKeyFrames.emplace_back();

                keyFrame.time = static_cast<float>(channel->mScalingKeys[k].mTime);
                keyFrame.transform = ToVec3(channel->mScalingKeys[k].mValue);
            }
        }
    }

    glm::quat OpenGLRenderer::InterpolateRotation(const Track& track, float time)
    {
        if (track.rotationKeyFrames.empty())
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity

        // if time is before the first key
        if (time <= track.rotationKeyFrames.front().time || track.rotationKeyFrames.size() == 1)
            return track.rotationKeyFrames.front().transform;

        // if time is after the last key
        if (time >= track.rotationKeyFrames.back().time)
            return track.rotationKeyFrames.back().transform;

        // find the two keys around `time`
        size_t i = 0;
        while (i < track.rotationKeyFrames.size() - 1 && time >= track.rotationKeyFrames[i + 1].time)
            i++;

        const auto& k1 = track.rotationKeyFrames[i];
        const auto& k2 = track.rotationKeyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);

        return glm::slerp(k1.transform, k2.transform, factor);
    }

    glm::vec3 OpenGLRenderer::InterpolateScale(const Track& track, float time)
    {
        if (track.scaleKeyFrames.empty())
            return glm::vec3(1.0f); // identity

        // if time is before the first key
        if (time <= track.scaleKeyFrames.front().time || track.scaleKeyFrames.size() == 1)
            return track.scaleKeyFrames.front().transform;

        // if time is after the last key
        if (time >= track.scaleKeyFrames.back().time)
            return track.scaleKeyFrames.back().transform;
        
        // find the two keys around `time`
        size_t i = 0;
        while (i < track.scaleKeyFrames.size() - 1 && time >= track.scaleKeyFrames[i + 1].time)
            i++;

        const auto& k1 = track.scaleKeyFrames[i];
        const auto& k2 = track.scaleKeyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);
        
        return glm::lerp(k1.transform, k2.transform, factor);
    }

    glm::vec3 OpenGLRenderer::InterpolatePosition(const Track& track, float time)
    {
        if (track.positionKeyFrames.empty())
            return glm::vec3{}; // identity

        // if time is before the first key
        if (time <= track.positionKeyFrames.front().time || track.positionKeyFrames.size() == 1)
            return track.positionKeyFrames.front().transform;

        // if time is after the last key
        if (time >= track.positionKeyFrames.back().time)
            return track.positionKeyFrames.back().transform;

        // find the two keys around `time`
        size_t i = 0;
        while (i < track.positionKeyFrames.size() - 1 && time >= track.positionKeyFrames[i + 1].time)
            i++;

        const auto& k1 = track.positionKeyFrames[i];
        const auto& k2 = track.positionKeyFrames[i + 1];

        float factor = (time - k1.time) / (k2.time - k1.time);

        return glm::lerp(k1.transform, k2.transform, factor);
    }

    // cleared once per call to load materials. used to map assimp material indexes to the internal mMaterials indexes
    static std::unordered_map<uint32_t, uint64_t> gAssimpMaterialIndexToMaterialIndex;

    // moves all data from the aiScene into the internal model format
    void OpenGLRenderer::LoadMaterials(const std::filesystem::path& path, const aiScene* scene)
    {
        gAssimpMaterialIndexToMaterialIndex.clear();

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            Gep::Material material;
            const aiMaterial* assimpMaterial = scene->mMaterials[i];

            aiColor3D outColor(1.f, 1.f, 1.f);
            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, outColor))
                material.color = { outColor.r, outColor.g, outColor.b, 1.0f };
            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, outColor))
                material.ao = outColor.r;
            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, outColor))
                material.metalness = outColor.r;
            if (aiReturn_SUCCESS == assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, outColor))
                material.roughness = outColor.r;

            material.diffuseTexture   = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_DIFFUSE);
            material.aoTexture        = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_AMBIENT_OCCLUSION);
            material.metalnessTexture = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_METALNESS);
            material.roughnessTexture = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_DIFFUSE_ROUGHNESS);
            material.normalTexture    = LoadTexturesFromAssimpMaterial(path, assimpMaterial, scene, aiTextureType_NORMALS);

            if (material.diffuseTexture.id)
                AddTexture(material.diffuseTexture);
            if (material.aoTexture.id)
                AddTexture(material.aoTexture);
            if (material.metalnessTexture.id)
                AddTexture(material.metalnessTexture);
            if (material.roughnessTexture.id)
                AddTexture(material.roughnessTexture);
            if (material.normalTexture.id)
                AddTexture(material.normalTexture);

            gAssimpMaterialIndexToMaterialIndex[i] = AddMaterial(material);
        }
    }

    void OpenGLRenderer::LoadAnimations(const std::string& name, Gep::Model& model, const aiScene* scene)
    {
        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            LoadAnimation(name, scene->mAnimations[i], model.skeleton);
        }
    }

    static void LoadVertices(Gep::Mesh& mesh, const aiMesh* assimpMesh)
    {
        mesh.vertices.reserve(assimpMesh->mNumVertices);

        for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i)
        {
            Vertex& v = mesh.vertices.emplace_back();

            v.position = { assimpMesh->mVertices[i].x, assimpMesh->mVertices[i].y, assimpMesh->mVertices[i].z };

            if (assimpMesh->HasNormals())
                v.normal = { assimpMesh->mNormals[i].x, assimpMesh->mNormals[i].y, assimpMesh->mNormals[i].z };

            if (assimpMesh->HasTextureCoords(0))
                v.texCoord = { assimpMesh->mTextureCoords[0][i].x, assimpMesh->mTextureCoords[0][i].y };
        }
    }

    static void LoadIndices(Gep::Mesh& mesh, const aiMesh* assimpMesh)
    {
        for (unsigned int i = 0; i < assimpMesh->mNumFaces; ++i)
        {
            const aiFace& face = assimpMesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                mesh.indices.push_back(face.mIndices[j]);
        }
    }

    // returns the index of the node just created
    static uint32_t LoadHierarchyStep(Gep::Model& model, const uint32_t parentIndex, const aiNode* node)
    {
        // if the passed node is null return num max signaling that this is a leaf
        if (!node) 
            return NumMax<uint32_t>();

        auto it = gBoneData.find(node->mName.C_Str());

        // if node is a bone sets it inverse bind otherwise leave as identity
        VQS inverseBind{};
        const bool isRealBone = it != gBoneData.end();
        if (isRealBone)
            inverseBind = it->second.offset;
        
        // create an entry in the heirarchy. 
        uint32_t index = model.skeleton.bones.size();
        Gep::Bone& bone = model.skeleton.bones.emplace_back();
        bone.name = node->mName.C_Str();
        bone.parentIndex = parentIndex;
        bone.transformation = ToVQS(node->mTransformation);
        bone.inverseBind = inverseBind;
        bone.isRealBone = isRealBone;

        // if its a bone add the index to the name association. Used when extracting vertex weights
        if (isRealBone) 
            it->second.index = index;

        // do the same thing for each child
        for (const aiNode* childNode : std::span(node->mChildren, node->mNumChildren))
        {
            uint32_t childIndex = LoadHierarchyStep(model, index, childNode);
            if (childIndex != NumMax<uint32_t>())
            {
                //note: cant get a reference here because it could be stale after recursive calls
                model.skeleton.bones.at(index).childrenIndices.push_back(childIndex);
            }
        }

        return index;
    }

    // create hierary
    static void LoadHierarchy(Gep::Model& model, const aiScene* scene)
    {
        // on the off chance a model doesn't have a root node?
        //uint32_t index = model.skeleton.bones.size();
        //Gep::Bone& bone = model.skeleton.bones.emplace_back();
        //bone.name = "Root";
        //bone.parentIndex = NumMax<uint32_t>();
        //bone.transformation = Gep::VQS{};
        //bone.inverseBind = Gep::VQS{};
        //bone.isRealBone = false;

        LoadHierarchyStep(model, NumMax<uint32_t>(), scene->mRootNode);
    }

    static void SetVertexBoneData(Vertex& vertex, uint32_t boneID, float weight)
    {
        for (int i = 0; i < vertex.boneIndices.size(); ++i)
        {
            if (vertex.boneIndices[i] == Vertex::INVALID_INDEX)
            {
                vertex.boneWeights[i] = weight;
                vertex.boneIndices[i] = boneID;
                break;
            }
        }
    }

    static void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* assimpMesh, const aiScene* scene)
    {
        for (const aiBone* assimpBone : std::span(assimpMesh->mBones, assimpMesh->mNumBones))
        {
            const std::string boneName = assimpBone->mName.C_Str();
            const uint32_t boneID = gBoneData.at(boneName).index; // index into the final bone heirarchy

            for (const aiVertexWeight assimpWeight : std::span(assimpBone->mWeights, assimpBone->mNumWeights))
            {
                const uint32_t vertexId = assimpWeight.mVertexId;
                const float weight = assimpWeight.mWeight;
                
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }

    static void LoadMeshes(Gep::Model& model, const aiScene* scene)
    {
        model.meshes.reserve(scene->mNumMeshes);

        for (const aiMesh* assimpMesh : std::span(scene->mMeshes, scene->mNumMeshes))
        {
            Mesh& mesh = model.meshes.emplace_back();
            mesh.name = assimpMesh->mName.C_Str();

            LoadVertices(mesh, assimpMesh);
            LoadIndices(mesh, assimpMesh);
            mesh.CalculateBoundingBox(); //must be done after vertices are loaded

            mesh.materialIndex = gAssimpMaterialIndexToMaterialIndex.at(assimpMesh->mMaterialIndex);
            ExtractBoneWeightForVertices(mesh.vertices, assimpMesh, scene);
        }
    }

    // maps the name of every bone to its inverse bind transformation
    // also used for checking existance of a bone
    static void LoadBoneData(const aiScene* scene)
    {
        for (const aiMesh* mesh : std::span(scene->mMeshes, scene->mNumMeshes))
        {
            for (const aiBone* bone : std::span(mesh->mBones, mesh->mNumBones))
            {
                const std::string name = bone->mName.C_Str();
                gBoneData[name].offset = ToVQS(bone->mOffsetMatrix);
            }
        }
    }

    Model OpenGLRenderer::LoadModelFromFile(const std::filesystem::path& path)
    {
        gBoneData.clear();
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs |
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

        Gep::Model model;

        model.name = path.string();

        // loads all of the materials out of this scene
        LoadMaterials(path, scene);

        // loads every bone name to its offset matrix in gBoneData
        LoadBoneData(scene);

        // fills in the skeleton of the model and the index field in gBoneData
        LoadHierarchy(model, scene);

        LoadMeshes(model, scene); //Broken?

        LoadAnimations(path.string(), model, scene);

        return model;
    }

    Texture OpenGLRenderer::EquirectangularToCubemap(const Texture& texture)
    {
        const uint32_t captureResolution = 512;

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 capturePVs[] =
        {
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        GLuint captureFBO;
        GLuint captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, captureResolution, captureResolution);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        Texture cubeMap;
        glGenTextures(1, &cubeMap.id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap.id);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, captureResolution, captureResolution, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // pbr: convert HDR equirectangular environment map to cubemap equivalent

        mShader_EquirectangularToCubemap.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.id);

        GLDrawFlags flags{
            .depthFuncMask = std::make_pair(GL_LEQUAL, GL_TRUE),
            .cullMode = std::nullopt,
            .blendFuncSD = std::nullopt
        };
        SetDrawFlags(flags);


        glViewport(0, 0, captureResolution, captureResolution); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            mShader_EquirectangularToCubemap.SetUniform("u_capturePV", capturePVs[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap.id, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // draw cube

            auto& cubeHandle = mMeshLibrary.at(mCubeMeshIndex).handle;
            glBindVertexArray(cubeHandle.mVertexArrayObject);
            mStats.drawCalls++;
            glDrawElements(
                GL_TRIANGLES,
                cubeHandle.mIndexCount,
                GL_UNSIGNED_INT,
                nullptr
            );

            glBindVertexArray(0); // holy shit found it. make sure to unbind vao
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);

        return cubeMap;
    }

    Texture OpenGLRenderer::CubemapToEquirectangular(const Texture& cubemap)
    {
        Texture equirectangular;
        glGenTextures(1, &equirectangular.id);

        glBindTexture(GL_TEXTURE_2D, equirectangular.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 512, 512, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLuint captureFBO;
        GLuint captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, equirectangular.id, 0);
        glViewport(0, 0, 512, 512);
        
        mShader_CubemapToEquirectangular.Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(mLineVAO); // core profile requires a VAO bound even when using gl_VertexID
        mStats.drawCalls++;
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return equirectangular;
    }

    Texture OpenGLRenderer::GenerateBRDFLUT()
    {
        const uint32_t brdflutSize = 512;
        const uint32_t captureSize = 512;
        const uint32_t sampleCount = 1024;

        mShader_GenerateBRDFLUT.SetUniform("u_sampleCount", sampleCount);

        Texture brdflut;
        glGenTextures(1, &brdflut.id);

        glBindTexture(GL_TEXTURE_2D, brdflut.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdflutSize, brdflutSize, 0, GL_RG, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLuint captureFBO;
        GLuint captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, captureSize, captureSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdflut.id, 0);
        glViewport(0, 0, 512, 512);
        mShader_GenerateBRDFLUT.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(mLineVAO); // core profile requires a VAO bound even when using gl_VertexID
        mStats.drawCalls++;
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return brdflut;
    }

    void OpenGLRenderer::GLDraw(GLuint vao, uint32_t indexCount, uint32_t instanceCount, uint32_t objectBaseInstance)
    {
        if (instanceCount < 1)
            return;
        if (indexCount < 1)
        {
            Gep::Log::Error("Attempting to draw with no indicies");
            return;
        }
        if (vao == 0)
        {
            Gep::Log::Error("Attempting to draw with an invalid vao");
            return;
        }

        mStats.drawCalls++;
        glBindVertexArray(vao);
        glDrawElementsInstancedBaseInstance(
            GL_TRIANGLES,
            indexCount,
            GL_UNSIGNED_INT,
            nullptr,
            instanceCount,
            objectBaseInstance
        );
        glBindVertexArray(0);
    }

    void OpenGLRenderer::GLDrawQuad()
    {
        mStats.drawCalls++;
        glBindVertexArray(mLineVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    Texture OpenGLRenderer::GeneratePrefilterMap(const Texture& environmentCubemap)
    {
        const uint32_t envMapFaceResolution = 512;
        const uint32_t faceResolution = 128;
        const uint32_t mipLevels = 5;
        const uint32_t sampleCount = 1024;

        mShader_Prefilter.SetUniform("u_faceResolution", faceResolution);
        mShader_Prefilter.SetUniform("u_sampleCount", sampleCount);

        // make the prefilter cubemap for pbr specular ibl
        Texture prefilterMap;
        glGenTextures(1, &prefilterMap.id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.id);
        for (uint32_t i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, faceResolution, faceResolution, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // make a new capture FBO and RBO to render the prefilter cubemap faces to
        GLuint captureFBO;
        GLuint captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, envMapFaceResolution, envMapFaceResolution);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 capturePVs[] =
        {
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        mShader_Prefilter.Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubemap.id);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        for (uint32_t mip = 0; mip < mipLevels; ++mip)
        {
            uint32_t mipWidth = faceResolution * std::pow(0.5, mip);
            uint32_t mipHeight = faceResolution * std::pow(0.5, mip);

            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(mipLevels - 1);
            mShader_Prefilter.SetUniform("u_roughness", roughness);

            for (uint32_t face = 0; face < 6; ++face)
            {
                mShader_Prefilter.SetUniform("u_capturePV", capturePVs[face]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, prefilterMap.id, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                auto& cubeHandle = mMeshLibrary.at(mCubeMeshIndex).handle;
                glBindVertexArray(cubeHandle.mVertexArrayObject);
                mStats.drawCalls++;
                glDrawElements(
                    GL_TRIANGLES,
                    cubeHandle.mIndexCount,
                    GL_UNSIGNED_INT,
                    nullptr
                );

                glBindVertexArray(0); // holy shit found it. make sure to unbind vao
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);

        return prefilterMap;
    }

    Texture OpenGLRenderer::GenerateIrradianceMap(const Texture& environmentCubemap)
    {
        uint32_t captureSize = 32;

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 capturePVs[] =
        {
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            captureProjection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        // make a new capture FBO and RBO to render the prefilter cubemap faces to
        GLuint captureFBO;
        GLuint captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, captureSize, captureSize);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        Texture irradianceMap;
        glGenTextures(1, &irradianceMap.id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap.id);
        for (uint32_t i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, captureSize, captureSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        mShader_GenerateIrradianceMap.Bind();
        mShader_GenerateIrradianceMap.SetUniform("u_environmentMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubemap.id);

        glViewport(0, 0, captureSize, captureSize);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (uint32_t i = 0; i < 6; ++i)
        {
            mShader_GenerateIrradianceMap.SetUniform("u_capturePV", capturePVs[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap.id, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto& cubeHandle = mMeshLibrary.at(mCubeMeshIndex).handle;
            glBindVertexArray(cubeHandle.mVertexArrayObject);
            mStats.drawCalls++;
            glDrawElements(
                GL_TRIANGLES,
                cubeHandle.mIndexCount,
                GL_UNSIGNED_INT,
                nullptr
            );

            glBindVertexArray(0); // holy shit found it. make sure to unbind vao
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);

        return irradianceMap;
    }
}
