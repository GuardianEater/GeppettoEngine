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

#include "Transform.hpp"
#include "ModelComponent.hpp"
#include "CameraComponent.hpp"
#include "TextureComponent.hpp"
#include "LightComponent.hpp"
#include "SkyboxMesh.hpp"

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

        renderer.SetShader("assets\\shaders\\PBR.vert", "assets\\shaders\\PBR.frag");
        renderer.SetHighlightShader("assets\\shaders\\Highlight.vert", "assets\\shaders\\Highlight.frag");

        renderer.SetLineShader("assets\\shaders\\Line.vert", "assets\\shaders\\Line.frag");

        renderer.SetUpLightSSBO();
        renderer.SetUpObjectUniformsSSBO();
        renderer.SetUpCameraUniformsSSBO();
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

    void RenderSystem::DrawSkeletonRecursive(const Gep::Skeleton& skeleton, const Gep::VQS& parentTransform, Gep::LineGPUData& line, uint16_t nodeIndex)
    {
        if (nodeIndex == Gep::num_max<uint16_t>()) // if the node is null
            return;
        if (skeleton.bones.empty()) // if no bones
            return;

        const Gep::Bone& bone = skeleton.bones.at(nodeIndex);

        Gep::VQS boneTransform = parentTransform * bone.transformation;

        // Draw lines to children
        for (uint16_t childIndex : bone.childrenIndices)
        {
            const Gep::Bone& child = skeleton.bones.at(childIndex);

            Gep::VQS childTransform = boneTransform * child.transformation;

            // Draw the bone (parent to child)
            line.points.push_back({ glm::vec4(boneTransform.position, 1.0f), glm::vec4(childTransform.position, 1.0f) });

            // Recurse
            DrawSkeletonRecursive(skeleton, boneTransform, line, childIndex);
        }
    }

    void RenderSystem::DrawSkeleton(const Gep::Skeleton& skeleton, const Gep::VQS& transform)
    {
        Gep::LineGPUData boneLines;
        boneLines.color = { 1.0f, 1.0f, 1.0f };
        DrawSkeletonRecursive(skeleton, transform, boneLines, 0); // 0 is the root node of a skeleton
        mRenderer.AddLine(boneLines);
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
        mManager.ForEachArchetype<ModelComponent, Transform>([&](Gep::Entity entity, ModelComponent& model, Transform& transform)
        {
            glm::mat4 modelMatrix = transform.GetModelMatrix();
            glm::mat4 normal = glm::mat4(glm::mat3(Gep::affine_inverse(modelMatrix)));

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

            Gep::ObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrix = normal,
                .isUsingTexture = !mNoTextureMode,
                .isIgnoringLight = model.ignoreLight,
                .isSolidColor = false,
                .isWireframe = mWireframeMode,
                .material = material
            };

            if (model.selected)
            {
                Gep::ObjectGPUData wireframeUniforms = uniforms;
                wireframeUniforms.isWireframe = true;
                wireframeUniforms.material.color = { 1.0f, 1.0f, 0.0f, 0.2f };

                renderer.AddObject(model.name, wireframeUniforms);
            }

            if (mDrawBones)
            {
                const Gep::Model& internalModel = renderer.GetModel(model.name);
                DrawSkeleton(internalModel.skeleton, Gep::ToVQS(modelMatrix));
            }
            
            renderer.AddObject(model.name, uniforms);
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

                renderer.AddObject("Cube", uniforms);
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

                renderer.AddObject("Sphere", uniforms);
            });

        }

        // send all things added to the gpu
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

        ImGui::DragFloat("near plane", &camera.nearPlane, 0.1f, 0.1f, 10000.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("far plane", &camera.farPlane, 0.1f, camera.nearPlane, 10000.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("fov", &camera.fov, 0.1f, 0.001f, 179.999f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);

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


