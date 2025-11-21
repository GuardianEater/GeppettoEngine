/*****************************************************************//**
 * \file   RenderSystem.cpp
 * \brief  System that renders objects
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "pch.hpp"

#include "RenderSystem.hpp"
#include "SphereCollider.hpp"
#include "CubeCollider.hpp"
#include <ImGuizmo.h>

 // component
#include "Transform.hpp"
#include "ModelComponent.hpp"
#include "CameraComponent.hpp"
#include "TextureComponent.hpp"
#include "LightComponent.hpp"
#include "AnimationComponent.hpp"

#include "SkyboxMesh.hpp"
#include "Mesh.hpp"

#include "Conversion.h"

#include "QuadMesh.hpp"
#include "SphereMesh.hpp"
#include "IcosphereMesh.hpp"
#include "CubeMesh.hpp"

#include "ISystem.hpp"

namespace Client
{
    RenderSystem::RenderSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
    {

    }

    RenderSystem::~RenderSystem()
    {
    }

    void RenderSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<ModelComponent>>(this, &RenderSystem::OnModelAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<ModelComponent>>(this, &RenderSystem::OnModelEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Texture>>(this, &RenderSystem::OnTextureEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Light>>(this, &RenderSystem::OnLightEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Camera>>(this, &RenderSystem::OnCameraEditorRender);

        Gep::OpenGLRenderer& renderer = mRenderer;

        renderer.LoadShader("Highlight", "assets\\shaders\\Highlight.vert", "assets\\shaders\\Highlight.frag");
        renderer.LoadShader("Line", "assets\\shaders\\Line.vert", "assets\\shaders\\Line.frag");
        renderer.LoadShader("PBR-Static", "assets\\shaders\\PBR-Static.vert", "assets\\shaders\\PBR.frag");
        renderer.LoadShader("PBR-Skinned", "assets\\shaders\\PBR-Skinned.vert", "assets\\shaders\\PBR.frag");

        renderer.SetUpLineDrawing();

        renderer.LoadErrorTexture("assets\\textures\\Checker.jpg");

        // load all of the default meshes
        {
            Gep::Model quad;
            quad.meshes.push_back(Gep::QuadMesh());
            renderer.AddModel("Quad", quad);
        }
        {
            Gep::Model sphere;
            sphere.meshes.push_back(Gep::SphereMesh(10, 10));
            renderer.AddModel("Sphere", sphere);
        }
        {
            Gep::Model cube;
            cube.meshes.push_back(Gep::CubeMesh());
            renderer.AddModel("Cube", cube);
        }
        {
            Gep::Model icosphere;
            icosphere.meshes.push_back(Gep::IcosphereMesh(3));
            renderer.AddModel("Icosphere", icosphere);
        }
        {
            Gep::Model skybox;
            skybox.meshes.push_back(Gep::SkyboxMesh());
            renderer.AddModel("Skybox", skybox);
        }
        {
            Gep::Material defaultMat;
            renderer.AddMaterial(defaultMat);
        }

        glEnable(GL_DEPTH_TEST);
    }

    void RenderSystem::Update(float dt)
    {
        Gep::OpenGLRenderer& renderer = mRenderer;

        renderer.Start();

        AddLights();
        AddCameras();
        AddObjects();

        // draws colliders if on
        if (mDrawColliders)
        {
            AddColliders();
        }

        // send all things added to the gpu
        renderer.CommitBones();
        renderer.CommitLights();
        renderer.CommitCameras();
        renderer.CommitObjects();


        // main render passes
        size_t cameraIndex = 0;
        mManager.ForEachArchetype<Transform, Camera>([&](Gep::Entity camEntity, Transform& camTransform, Camera& cam)
        {
            renderer.SetCameraIndex(cameraIndex++);

            cam.renderTarget->Bind();
            cam.renderTarget->Clear({ 0.0f, 0.0f, 0.0f });

            // draw the scene once for every camera
            renderer.Draw();

            cam.renderTarget->Draw(mManager, camEntity);
            cam.Resize(cam.renderTarget->GetSize());
            cam.renderTarget->Unbind();
        });

        mManager.ForEachArchetype<ModelComponent, Transform>([&](Gep::Entity entity, ModelComponent& model, Transform& transform)
        {
            model.selected = false;
        });

        renderer.End();
        HandleInputs(dt);
    }

    void RenderSystem::FrameEnd()
    {
        // opengl clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderSystem::HandleInputs(float dt)
    {
        GLFWwindow* window = glfwGetCurrentContext();

        static bool isF1 = false;
        static bool isF2 = false;
        static bool isF3 = false;
        static bool isF4 = false;
        static bool isShaderReload = false;

        // for textures
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
        {
            if (!isF1)
            {
                mNoTextureMode = !mNoTextureMode;
                isF1 = true;
            }
        }
        else
            isF1 = false;

        // for wireframes
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
        {
            if (!isF2)
            {
                mWireframeMode = !mWireframeMode;
                isF2 = true;
            }
        }
        else
            isF2 = false; // Reset when key is released

        // for colliders
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
        {
            if (!isF3)
            {
                this->mDrawColliders = !this->mDrawColliders;
                isF3 = true;
            }
        }
        else
            isF3 = false; // Reset when key is released

        // for bones
        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS)
        {
            if (!isF4)
            {
                mDrawBones = !mDrawBones;
                isF4 = true;
            }
        }
        else
            isF4 = false; // Reset when key is released


        // for shader reloading
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
        {
            if (!isShaderReload)
            {
                Gep::Log::Info("Reloading Shaders...");
                mRenderer.ReloadShaders();
                
                isShaderReload = true;
            }
        }
        else
            isShaderReload = false; // Reset when key is released

    }

    void RenderSystem::RenderImGui(float dt)
    {
        ImGui::Begin("Render System");
        ImGui::End();
    }

    void RenderSystem::OnModelAdded(const Gep::Event::ComponentAdded<ModelComponent>& event)
    {
        Gep::OpenGLRenderer& renderer = mRenderer;

        ModelComponent& model = event.component;

        // if the model is not loaded when this component is added attempt load it
        if (!renderer.IsModelLoaded(model.name))
        {
            if (std::filesystem::exists(model.name))
            {
                renderer.AddModelFromFile(model.name);
            }
            else
            {
                const std::string defaultName = ModelComponent{}.name; // re-initializes the meshname to the default value
                Gep::Log::Warning("A model component was created with an invalid name/location: [", model.name, "] doesn't exist. It will be changed to the error mesh: [", defaultName, "] instead.");
                model.name = defaultName;
            }
        }

        if (renderer.IsModelLoaded(model.name))
        {
            const Gep::Model& internalModel = mRenderer.GetModel(model.name);

            model.pose.clear();
            model.pose.resize(internalModel.skeleton.bones.size());

            // initialize with the default skeleton
            for (size_t i = 0; i < model.pose.size(); ++i)
            {
                model.pose[i] = internalModel.skeleton.bones[i].transformation;
            }

            const size_t okayuArmIndex = 120;
            if (model.pose.size() > 200)
            {
                glm::mat4 m = Gep::ToMat4(model.pose[okayuArmIndex]);
                if (model.pose.size() > 200)
                    m = glm::rotate(m, glm::radians(70.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                model.pose[okayuArmIndex] = Gep::ToVQS(m);
            }

            // calculate the global pose
            for (uint32_t i = 1; i < internalModel.skeleton.bones.size(); i++) // note skip the root bone
            {
                uint32_t parent = internalModel.skeleton.bones[i].parentIndex;

                model.pose[i] = model.pose[parent] * model.pose[i];
            }
        }
    }

    void RenderSystem::OnModelEditorRender(const Gep::Event::ComponentEditorRender<ModelComponent>& event)
    {
        ModelComponent& mesh = event.component;

        Gep::OpenGLRenderer& renderer = mRenderer;
        Client::EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::string> loadedMeshes = renderer.GetLoadedMeshes();

        // drop down for selecting a model
        bool meshesOpen = ImGui::BeginCombo("Models", mesh.name.c_str());

        const std::vector<std::string>& allowedExtensions = renderer.GetSupportedModelFormats();

        er.AssetBrowserDropTarget(allowedExtensions, [&](const std::filesystem::path& droppedPath)
        {
            if (!renderer.IsModelLoaded(droppedPath.string()))
            {
                renderer.AddModelFromFile(droppedPath.string());
            }

            mesh.name = droppedPath.string();
        });

        if (meshesOpen)
        {
            for (const std::string& meshName : loadedMeshes)
            {
                bool isSelected = (meshName == mesh.name);
                if (ImGui::Selectable(meshName.c_str(), isSelected))
                {
                    mesh.name = meshName;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Ignore Light", &mesh.ignoreLight);
    }

    void RenderSystem::OnTextureEditorRender(const Gep::Event::ComponentEditorRender<Texture>& event)
    {
        Texture& texture = event.component;

        const Gep::OpenGLRenderer& renderer = mRenderer;
        const Client::EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::filesystem::path> loadedTextures = renderer.GetLoadedTextures();

        bool texturesOpen = ImGui::BeginCombo("Textures", texture.texturePath.string().c_str());

        const std::vector<std::string>& supportedTextures = renderer.GetSupportedTextureFormats();

        er.AssetBrowserDropTarget(supportedTextures, [&](const std::filesystem::path& droppedPath)
        {
            texture.texturePath = droppedPath;
        });

        if (texturesOpen)
        {
            for (const auto& loadedTexturePath : loadedTextures)
            {
                bool isSelected = loadedTexturePath == texture.texturePath;
                if (ImGui::Selectable(loadedTexturePath.string().c_str(), isSelected))
                {
                    texture.texturePath = loadedTexturePath;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void RenderSystem::OnLightEditorRender(const Gep::Event::ComponentEditorRender<Light>& event)
    {
        Light& light = event.component;

        ImGui::ColorEdit3("Color", &light.color[0]);
        ImGui::DragFloat("Intensity", &light.intensity, 1.0f, 0.001f, Gep::num_max<float>());
    }

    void RenderSystem::OnCameraEditorRender(const Gep::Event::ComponentEditorRender<Camera>& event)
    {
        Camera& camera = event.component;
        EditorResource& er = mManager.GetResource<EditorResource>();

        er.LabledInput_Float("Near Plane", &camera.nearPlane, 100.0f, 0.1f, 0.1f, 10000.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        er.LabledInput_Float("Far Plane", &camera.farPlane, 100.0f, 0.1f, camera.nearPlane, 10000.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        er.LabledInput_Float("FOV", &camera.fov, 100.0f, 0.1f, 0.001f, 179.999f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);

        // drop down menu for render target
        if (ImGui::BeginCombo("target", camera.renderTargetType.PrettyName().c_str()))
        {
            if (ImGui::Selectable("Imgui"))
            {
                camera.renderTarget = std::make_shared<Gep::RenderTargetImgui>(500, 500);
                camera.renderTargetType = Gep::GetTypeInfo<Gep::RenderTargetImgui>();
            }
            if (ImGui::Selectable("Window"))
            {
                camera.renderTarget = std::make_shared<Gep::RenderTargetWindow>(500, 500);
                camera.renderTargetType = Gep::GetTypeInfo<Gep::RenderTargetWindow>();
            }
            ImGui::EndCombo();
        }
    }

    void RenderSystem::AddColliders()
    {
        mManager.ForEachArchetype<CubeCollider, Transform>([&](Gep::Entity entity, CubeCollider& collider, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);

            Gep::ObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2]
            };

            mRenderer.AddObject("PBR-Static", "Cube", uniforms, Gep::RenderFlags::Wireframe);
        });

        mManager.ForEachArchetype<SphereCollider, Transform>([&](Gep::Entity entity, SphereCollider& collider, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);

            Gep::ObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2]
            };

            mRenderer.AddObject("PBR-Static", "Sphere", uniforms, Gep::RenderFlags::Wireframe);
        });
    }

    void RenderSystem::AddLights()
    {
        // prepares all of the light uniform values
        mManager.ForEachArchetype<Light, Transform>([&](Gep::Entity e, Light& l, Transform& t)
        {
            Gep::LightGPUData uniforms
            {
                .position = t.world.position,
                .color = l.color,
                .intensity = l.intensity
            };

            mRenderer.AddLight(uniforms);
        });
    }

    void RenderSystem::AddCameras()
    {
        // prepares the camera uniforms
        mManager.ForEachArchetype<Transform, Camera>([&](Gep::Entity camEntity, Transform& camTransform, Camera& cam)
        {
            Gep::CameraGPUData uniforms
            {
                .perspectiveMatrix = cam.GetProjectionMatrix(),
                .viewMatrix = cam.GetViewMatrix(camTransform.world.position),
                .camPosition = glm::vec4(camTransform.world.position, 1.0f),
            };

            mRenderer.AddCamera(uniforms);
        });
    }

    static void DrawSkeleton(const Gep::Skeleton& skeleton, const glm::mat4& modelMatrix, const std::vector<Gep::VQS>& globalPose, Gep::LineGPUData& line)
    {
        // note skipping 0 because zero is guarenteed to be the root node
        for (size_t i = 1; i < skeleton.bones.size() && i < globalPose.size(); i++)
        {
            uint32_t parent = skeleton.bones[i].parentIndex;
            glm::vec4 a = glm::vec4(globalPose[parent].position, 1.0f);
            glm::vec4 b = glm::vec4(globalPose[i].position, 1.0f);

            a = modelMatrix * a;
            b = modelMatrix * b;

            line.points.push_back({ a, b }); // however you render debug lines
        }
    }

    void RenderSystem::AddObjects()
    {
        Gep::LineGPUData skeletonLines;
        skeletonLines.color = { 1.0f, 0.5f, 0.5f };
        int boneOffset = 0;

        mManager.ForEachArchetype<ModelComponent, Transform>([&](Gep::Entity entity, ModelComponent& model, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);
            const Gep::Model& internalModel = mRenderer.GetModel(model.name);

            if (mManager.HasComponent<Light>(entity))
            {
                model.ignoreLight = true;
            }

            std::string targetShader = "PBR-Static";

            // if the model also has an animation compute its final pose and pass all bone info to the gpu
            int previousBoneOffset = boneOffset;

            for (uint32_t i = 0; i < model.pose.size() && i < internalModel.skeleton.bones.size(); ++i)
            {
                Gep::BoneGPUData bone{
                    .offsetMatrix = Gep::ToMat4(model.pose[i] * internalModel.skeleton.bones[i].inverseBind)
                };

                mRenderer.AddBone(bone);
                ++boneOffset;
                targetShader = "PBR-Skinned";
            }


            if (mDrawBones)
            {
                DrawSkeleton(internalModel.skeleton, modelMatrix, model.pose, skeletonLines);
            }


            Gep::ObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2],

                .boneOffset = previousBoneOffset // only used in the skinned pbr shader
            };

            Gep::RenderFlags flags = Gep::RenderFlags::None;
            if (mWireframeMode)
                flags |= Gep::RenderFlags::Wireframe;

            if (model.selected)
                flags |= Gep::RenderFlags::Highlight;

            mRenderer.AddObject(targetShader, model.name, uniforms, flags);
        });

        mRenderer.AddLine(skeletonLines);

    }
}


