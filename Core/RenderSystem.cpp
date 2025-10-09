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

        renderer.SetUpLightSSBO();
        renderer.SetUpObjectUniformsSSBO();
        renderer.SetUpCameraUniformsSSBO();
        renderer.SetUpBoneUniformsSSBO();
        renderer.SetUpLineDrawing();

        renderer.BackfaceCull();

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

        glEnable(GL_DEPTH_TEST);
    }

    static void DrawSkeleton(const Gep::Skeleton& skeleton, const glm::mat4& modelMatrix, const std::vector<Gep::VQS>& globalPose, Gep::LineGPUData& line)
    {
        for (int i = 0; i < skeleton.bones.size(); i++)
        {
            int parent = skeleton.bones[i].parentIndex;
            if (parent != Gep::num_max<uint16_t>())
            {
                glm::vec4 a = glm::vec4(globalPose[parent].position, 1.0f);
                glm::vec4 b = glm::vec4(globalPose[i].position, 1.0f);

                a = modelMatrix * a;
                b = modelMatrix * b;

                line.points.push_back({ a, b }); // however you render debug lines
            }
        }
    }

    void RenderSystem::Update(float dt)
    {
        Gep::OpenGLRenderer& renderer = mRenderer;

        renderer.Start();

        // prepares all of the light uniform values
        mManager.ForEachArchetype<Light, Transform>([&](Gep::Entity e, Light& l, Transform& t)
            {
                Gep::LightGPUData uniforms
                {
                    .position = t.position,
                    .color = l.color,
                    .intensity = l.intensity
                };

                renderer.AddLight(uniforms);
            });

        // prepares the camera uniforms
        mManager.ForEachArchetype<Transform, Camera>([&](Gep::Entity camEntity, Transform& camTransform, Camera& cam)
            {
                Gep::CameraGPUData uniforms
                {
                    .perspectiveMatrix = cam.GetProjectionMatrix(),
                    .viewMatrix = cam.GetViewMatrix(camTransform.position),
                    .camPosition = glm::vec4(camTransform.position, 1.0f),
                };

                // convert the camera's rotation to radians
                glm::vec3 camRotation = glm::radians(camTransform.rotation);

                // calculate the camera's right, up, and back vectors from the transforms rotation
                cam.right = { cos(camRotation.y), 0, sin(camRotation.y) };
                cam.up = { sin(camRotation.x) * sin(camRotation.y), cos(camRotation.x), -sin(camRotation.x) * cos(camRotation.y) };
                cam.back = glm::normalize(glm::cross(cam.right, cam.up));

                renderer.AddCamera(uniforms);
            });

        // prepare the object uniforms
        Gep::LineGPUData skeletonLines;
        skeletonLines.color = { 1.0f, 0.5f, 0.5f };
        int boneOffset = 0;

        mManager.ForEachArchetype<ModelComponent, Transform>([&](Gep::Entity entity, ModelComponent& model, Transform& transform)
            {
                const glm::mat4 modelMatrix = transform.GetModelMatrix();
                const glm::mat4 normal = glm::mat4(glm::mat3(Gep::affine_inverse(modelMatrix)));
                const Gep::Model& internalModel = renderer.GetModel(model.name);

                Gep::MaterialGPUData material
                {
                    .ao = model.ao, // ambient occlusion
                    .roughness = model.roughness,
                    .metalness = model.metalness,
                    .color = glm::vec4(model.color, 1.0f)
                };

                if (mManager.HasComponent<Light>(entity))
                {
                    material.color = glm::vec4(mManager.GetComponent<Light>(entity).color, 1.0f);
                    model.ignoreLight = true;
                }

                std::string targetShader = "PBR-Static";

                // if the model also has an animation compute its final pose and pass all bone info to the gpu
                int previousBoneOffset = boneOffset;
                if (mManager.HasComponent<AnimationComponent>(entity) && mManager.IsState(Gep::EngineState::Play))
                {
                    AnimationComponent& ac = mManager.GetComponent<AnimationComponent>(entity);

                    for (uint32_t i = 0; i < internalModel.skeleton.bones.size() && i < ac.pose.size(); ++i)
                    {
                        Gep::BoneGPUData bone{
                            .offsetMatrix = Gep::ToMat4(ac.pose[i] * internalModel.skeleton.bones[i].inverseBind)
                        };

                        mRenderer.AddBone(bone);
                        ++boneOffset;
                    }

                    if (mDrawBones)
                    {
                        DrawSkeleton(internalModel.skeleton, modelMatrix, ac.pose, skeletonLines);
                    }

                    targetShader = "PBR-Skinned";
                }

                Gep::ObjectGPUData uniforms
                {
                    .modelMatrix = modelMatrix,
                    .normalMatrix = normal,
                    .isUsingTexture = !mNoTextureMode,
                    .isIgnoringLight = model.ignoreLight,
                    .isSolidColor = false,
                    .isWireframe = mWireframeMode,
                    .material = material,
                    .boneOffset = previousBoneOffset // only used in the skinned pbr shader
                };

                if (model.selected)
                {
                    Gep::ObjectGPUData wireframeUniforms = uniforms;
                    wireframeUniforms.isWireframe = true;
                    wireframeUniforms.material.color = { 1.0f, 1.0f, 0.0f, 0.2f };

                    renderer.AddObject("PBR-Static", model.name, wireframeUniforms, Gep::RenderFlags::Wireframe | Gep::RenderFlags::NoDepthTest);
                }

                renderer.AddObject(targetShader, model.name, uniforms);
            });

        // draws colliders if on

        if (mDrawColliders)
        {
            Gep::MaterialGPUData material
            {
                .ao = 1.0f, // ambient occlusion
                .roughness = 1.0f,
                .metalness = 1.0f,
                .color = {1.0f, 0.0f, 0.0f, 0.5f}
            };

            mManager.ForEachArchetype<CubeCollider, Transform>([&](Gep::Entity entity, CubeCollider& collider, Transform& transform)
                {
                    glm::mat4 modelMatrix = transform.GetModelMatrix();
                    glm::mat4 normal = glm::mat4(glm::mat3(Gep::affine_inverse(modelMatrix)));

                    Gep::ObjectGPUData uniforms
                    {
                        .modelMatrix = modelMatrix,
                        .normalMatrix = normal,
                        .isUsingTexture = false,
                        .isIgnoringLight = true,
                        .isSolidColor = true,
                        .isWireframe = true,
                        .material = material
                    };

                    renderer.AddObject("PBR-Static", "Cube", uniforms, Gep::RenderFlags::Wireframe);
                });

            mManager.ForEachArchetype<SphereCollider, Transform>([&](Gep::Entity entity, SphereCollider& collider, Transform& transform)
                {
                    glm::mat4 modelMatrix = transform.GetModelMatrix();
                    glm::mat4 normal = glm::mat4(glm::mat3(Gep::affine_inverse(modelMatrix)));

                    Gep::ObjectGPUData uniforms
                    {
                        .modelMatrix = modelMatrix,
                        .normalMatrix = normal,
                        .isUsingTexture = false,
                        .isIgnoringLight = true,
                        .isSolidColor = true,
                        .isWireframe = true,
                        .material = material
                    };

                    renderer.AddObject("PBR-Static", "Sphere", uniforms, Gep::RenderFlags::Wireframe);
                });

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
        Gep::OpenGLRenderer& renderer = mRenderer;

        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        const float movementSpeed = 10 * dt;

        GLFWwindow* window = glfwGetCurrentContext();

        static bool isTKeyPressed = false;
        static bool isYKeyPressed = false;
        static bool isUKeyPressed = false;
        static bool isFKeyPressed = false;

        // Handle T key for textures
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
        {
            if (!isTKeyPressed)
            {
                mNoTextureMode = !mNoTextureMode;
                isTKeyPressed = true;
            }
        }
        else
            isTKeyPressed = false;

        // Handle Y key for wireframes
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
        {
            if (!isYKeyPressed)
            {
                mWireframeMode = !mWireframeMode;
                isYKeyPressed = true;
            }
        }
        else
            isYKeyPressed = false; // Reset when key is released

        // Handle U key for wireframes
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
        {
            if (!isUKeyPressed)
            {
                this->mDrawColliders = !this->mDrawColliders;
                isUKeyPressed = true;
            }
        }
        else
            isUKeyPressed = false; // Reset when key is released

        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS)
        {
            if (!isFKeyPressed)
            {
                mDrawBones = !mDrawBones;
                isFKeyPressed = true;
            }
        }
        else
            isFKeyPressed = false; // Reset when key is released

    }

    void RenderSystem::RenderImGui(float dt)
    {
        ImGui::Begin("Render System");
        ImGui::End();
    }

    void RenderSystem::OnModelAdded(const Gep::Event::ComponentAdded<ModelComponent>& event)
    {
        Gep::OpenGLRenderer& renderer = mRenderer;

        ModelComponent& model = mManager.GetComponent<ModelComponent>(event.entity);

        if (!renderer.IsMeshLoaded(model.name))
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
                if (!renderer.IsMeshLoaded(droppedPath.string()))
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

        ImGui::DragFloat("Ambient Occlusion", &mesh.ao, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Roughness", &mesh.roughness, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Metalness", &mesh.metalness, 0.001f, 0.0f, 1.0f);
        ImGui::ColorEdit3("Color", &mesh.color[0]);
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
}


