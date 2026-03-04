/*****************************************************************//**
 * \file   RenderSystem.cpp
 * \brief  System that renders objects
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

// pch
#include "pch.hpp"

// this
#include "RenderSystem.hpp"

 // component
#include "Transform.hpp"
#include "ModelComponent.hpp"
#include "CameraComponent.hpp"
#include "ShadowCasterComponent.hpp"
#include "LightComponent.hpp"
#include "AnimationComponent.hpp"
#include "SphereCollider.hpp"
#include "CubeCollider.hpp"

// mesh
#include "SkyboxMesh.hpp"
#include "Mesh.hpp"
#include "QuadMesh.hpp"
#include "SphereMesh.hpp"
#include "IcosphereMesh.hpp"
#include "CubeMesh.hpp"

// engine
#include "ISystem.hpp"

// resource
#include "OpenGLRenderer.hpp"
#include "EditorResource.hpp"
#include "CollisionResource.hpp"

// help
#include "Conversion.hpp"
#include "STLHelp.hpp"
#include "ImGuiHelp.hpp"
#include "GLMHelp.hpp"

namespace Client
{
    RenderSystem::RenderSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer(em.GetResource<Gep::OpenGLRenderer>())
        , mCollisionResource(em.GetResource<Client::CollisionResource>())
        , mEditorResource(em.GetResource<Client::EditorResource>())
    {

    }

    RenderSystem::~RenderSystem()
    {
    }

    void RenderSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<RiggedModelComponent>>(this, &RenderSystem::OnRiggedModelAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<StaticModelComponent>>(this, &RenderSystem::OnStaticModelAdded);

        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<RiggedModelComponent>>(this, &RenderSystem::OnRiggedModelEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<StaticModelComponent>>(this, &RenderSystem::OnStaticModelEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Light>>(this, &RenderSystem::OnPointLightEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<ShadowCasterComponent>>(this, &RenderSystem::OnShadowCasterEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<DirectionalLight>>(this, &RenderSystem::OnDirectionalLightEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Camera>>(this, &RenderSystem::OnCameraEditorRender);

        Gep::OpenGLRenderer& renderer = mRenderer;
        renderer.Initialize();

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
            Gep::MaterialGPUData defaultMat;
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
        renderer.CommitLights();
        renderer.CommitCameras();
        renderer.CommitBones();
        renderer.CommitObjects();


        // main render passes
        size_t cameraIndex = 0;
        mManager.ForEachArchetype([&](Gep::Entity camEntity, Transform& camTransform, Camera& cam)
        {
            renderer.SetCameraIndex(cameraIndex++);

            cam.renderTarget.Bind();
            cam.renderTarget.Clear();

            DrawImGuiCameraWindow(camEntity, cam, camTransform);

            // draw the scene once for every camera
            renderer.Draw(cam.renderTarget);

            cam.Resize(cam.renderTarget.GetSize());
            cam.renderTarget.Unbind();
        });

        mManager.ForEachArchetype([&](Gep::Entity entity, RiggedModelComponent& model, Transform& transform)
        {
            model.selected = false;
        });

        mManager.ForEachArchetype([&](Gep::Entity entity, StaticModelComponent& model, Transform& transform)
        {
            model.selected = false;
        });

        HandleInputs(dt);
    }

    void RenderSystem::FrameEnd()
    {
        mRenderer.End();
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

    void RenderSystem::OnRiggedModelAdded(const Gep::Event::ComponentAdded<RiggedModelComponent>& event)
    {
        Gep::OpenGLRenderer& renderer = mRenderer;

        RiggedModelComponent& model = event.component;

        // if the model is not loaded when this component is added attempt load it
        if (!renderer.IsModelLoaded(model.name))
        {
            if (std::filesystem::exists(model.name))
            {
                renderer.AddModelFromFile(model.name);
            }
            else
            {
                const std::string defaultName = RiggedModelComponent{}.name; // re-initializes the meshname to the default value
                Gep::Log::Warning("A model component was created with an invalid name/location: [", model.name, "] doesn't exist. It will be changed to the error mesh: [", defaultName, "] instead.");
                model.name = defaultName;
            }
        }

        const Gep::Model& internalModel = mRenderer.GetModel(model.name);

        InitializeModelPose(model, internalModel);
    }

    void RenderSystem::OnStaticModelAdded(const Gep::Event::ComponentAdded<StaticModelComponent>& event)
    {
        Gep::OpenGLRenderer& renderer = mRenderer;

        StaticModelComponent& model = event.component;

        // if the model is not loaded when this component is added attempt load it
        if (!renderer.IsModelLoaded(model.name))
        {
            if (std::filesystem::exists(model.name))
            {
                renderer.AddModelFromFile(model.name);
            }
            else
            {
                const std::string defaultName = RiggedModelComponent{}.name; // re-initializes the meshname to the default value
                Gep::Log::Warning("A model component was created with an invalid name/location: [", model.name, "] doesn't exist. It will be changed to the error mesh: [", defaultName, "] instead.");
                model.name = defaultName;
            }
        }
    }

    void RenderSystem::OnRiggedModelEditorRender(const Gep::Event::ComponentEditorRender<RiggedModelComponent>& event)
    {
        std::span<RiggedModelComponent*> models = event.components;

        Client::EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::string> loadedModels = mRenderer.GetLoadedModels();

        std::string selectedModelName = models[0]->name; // in this event call there is guaranteed to be at least one component

        // if all selected models have the same name, show it
        bool allSame = true;
        for (size_t i = 1; i < models.size(); ++i)
        {
            if (models[i]->name != selectedModelName)
            {
                allSame = false;
                break;
            }
        }

        // drop down for selecting a model
        bool modelsOpen = ImGui::BeginCombo("Models", allSame ? selectedModelName.c_str() : "-");

        const std::vector<std::string>& allowedExtensions = mRenderer.GetSupportedModelFormats();

        er.AssetBrowserDropTarget(allowedExtensions, [&](const std::filesystem::path& droppedPath)
        {
            if (!mRenderer.IsModelLoaded(droppedPath.string()))
            {
                mRenderer.AddModelFromFile(droppedPath.string());
            }

            for (RiggedModelComponent* model : models)
            {
                model->name = droppedPath.string();
                const Gep::Model& internalModel = mRenderer.GetModel(model->name);
                InitializeModelPose(*model, internalModel);
            }
        });

        if (modelsOpen)
        {
            for (const std::string& modelName : loadedModels)
            {
                const bool isSelected = allSame && modelName == selectedModelName;
                if (ImGui::Selectable(modelName.c_str(), isSelected))
                {
                    const Gep::Model& internalModel = mRenderer.GetModel(modelName);
                    for (RiggedModelComponent* model : models)
                    {
                        model->name = modelName;
                        InitializeModelPose(*model, internalModel);
                    }
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void RenderSystem::OnStaticModelEditorRender(const Gep::Event::ComponentEditorRender<StaticModelComponent>& event)
    {
        const std::span<StaticModelComponent*> models = event.components;

        Client::EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::string> loadedModels = mRenderer.GetLoadedModels();

        std::string selectedModelName = models[0]->name; // in this event call there is guaranteed to be at least one component

        // if all selected models have the same name, show it
        bool allSame = true;
        for (size_t i = 1; i < models.size(); ++i)
        {
            if (models[i]->name != selectedModelName)
            {
                allSame = false;
                break;
            }
        }

        // drop down for selecting a model
        bool modelsOpen = ImGui::BeginCombo("Models", allSame ? selectedModelName.c_str() : "-");

        const std::vector<std::string>& allowedExtensions = mRenderer.GetSupportedModelFormats();

        er.AssetBrowserDropTarget(allowedExtensions, [&](const std::filesystem::path& droppedPath)
            {
                if (!mRenderer.IsModelLoaded(droppedPath.string()))
                {
                    mRenderer.AddModelFromFile(droppedPath.string());
                }

                for (StaticModelComponent* model : models)
                {
                    model->name = droppedPath.string();
                }
            });

        if (modelsOpen)
        {
            for (const std::string& modelName : loadedModels)
            {
                const bool isSelected = allSame && modelName == selectedModelName;
                if (ImGui::Selectable(modelName.c_str(), isSelected))
                {
                    for (StaticModelComponent* model : models)
                    {
                        model->name = modelName;
                    }
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void RenderSystem::OnPointLightEditorRender(const Gep::Event::ComponentEditorRender<Light>& event)
    {
        std::span<Light*> lights = event.components;

        Gep::ImGui::MultiDragFloat3("Color", lights,
            [](Light* light) -> float& { return light->color.r; },
            [](Light* light) -> float& { return light->color.g; },
            [](Light* light) -> float& { return light->color.b; }
        );

        Gep::ImGui::MultiDragFloat("Intensity", lights,
            [](Light* light) -> float& { return light->intensity; }
        );

        Gep::ImGui::MultiCheckbox("Enabled", lights,
            [](Light* light) -> bool& { return light->enabled; }
        );
    }

    void RenderSystem::OnShadowCasterEditorRender(const Gep::Event::ComponentEditorRender<ShadowCasterComponent>& event)
    {
        std::span<ShadowCasterComponent*> scs = event.components;

        // only single selection beyond this point
        if (scs.size() > 1)
            return;

        ShadowCasterComponent& sc = *scs.front();

        ImGui::Text("This component makes lighting components cast shadows");
    }

    void RenderSystem::OnDirectionalLightEditorRender(const Gep::Event::ComponentEditorRender<DirectionalLight>& event)
    {
        std::span<DirectionalLight*> lights = event.components;

        Gep::ImGui::MultiDragFloat3("Color", lights,
            [](DirectionalLight* light) -> float& { return light->color.r; },
            [](DirectionalLight* light) -> float& { return light->color.g; },
            [](DirectionalLight* light) -> float& { return light->color.b; }
        );

        Gep::ImGui::MultiDragFloat("Intensity", lights,
            [](DirectionalLight* light) -> float& { return light->intensity; }
        );

        Gep::ImGui::MultiCheckbox("Enabled", lights,
            [](DirectionalLight* light) -> bool& { return light->enabled; }
        );
    }

    void RenderSystem::OnCameraEditorRender(const Gep::Event::ComponentEditorRender<Camera>& event)
    {
        std::span<Camera*> cameras = event.components;
        EditorResource& er = mManager.GetResource<EditorResource>();

        Gep::ImGui::MultiDragFloat("Near Plane", cameras, [](Camera* cam) -> float& { return cam->nearPlane; });
        Gep::ImGui::MultiDragFloat("Far Plane",  cameras, [](Camera* cam) -> float& { return cam->farPlane; });
        Gep::ImGui::MultiDragFloat("FOV",        cameras, [](Camera* cam) -> float& { return cam->fov; });


        bool uniform = Gep::IsUniform(cameras, [&](Camera* cam) { return cam->renderToImGui; });

        std::string displayName = uniform ? (cameras[0]->renderToImGui ? "Imgui" : "Window") : "-";

        // drop down menu for render target
        if (ImGui::BeginCombo("Target", displayName.c_str()))
        {
            if (ImGui::Selectable("Imgui"))
            {
                for (Camera* camera : cameras)
                {
                    camera->renderTarget = Gep::FrameBuffer::Create({ 500, 500 });
                    camera->renderToImGui = true;
                }
            }
            if (ImGui::Selectable("Window"))
            {
                for (Camera* camera : cameras)
                {
                    camera->renderTarget = Gep::FrameBuffer::Default();
                    camera->renderToImGui = false;
                }
            }
            ImGui::EndCombo();
        }
    }

    void RenderSystem::DrawImGuiCameraWindow(Gep::Entity cameraEntity, Client::Camera& camera, Client::Transform& cameraTransform)
    {
        const ImVec2 windowSizeMin(400, 300);
        const ImVec2 windowSizeMax(FLT_MAX, FLT_MAX);
        const float dt = mManager.GetDeltaTime();
        const float sensitivity = 0.1f;
        float movementSpeed = 25.0f * dt;
        float boostMuliplier = 4.0f;


        const glm::vec3 forward = glm::normalize(glm::vec3(-camera.back.x, 0.0f, -camera.back.z));
        const glm::vec3 rightward = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

        ImGui::SetNextWindowSize(windowSizeMin, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(windowSizeMin, windowSizeMax);
        std::string windowName = mManager.GetName(cameraEntity) + "###" + mManager.GetUUID(cameraEntity).to_string();
        if (ImGui::Begin(windowName.c_str()))
        {
            // get mouse delta and wether or not right click is pressed to rotate the camera
            bool rightClick = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            bool mouseRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
            bool mouseRightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
            static bool movementEnabled = false;
            ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
            bool focused = ImGui::IsWindowFocused();
            bool hovered = ImGui::IsWindowHovered();

            // gets the underlying window
            ImGuiViewport* viewport = ImGui::GetWindowViewport();
            GLFWwindow* window = nullptr;
            if (viewport && viewport->PlatformHandle)
                window = (GLFWwindow*)viewport->PlatformHandle;
            else
            {
                Gep::Log::Error("Failed to get window handle");
                ImGui::End();
                return;
            }

            // hide mouse and enable movement while right click is down
            if (mouseRightClicked && hovered)
            {
                movementEnabled = true;

                ImGui::SetWindowFocus();

                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else if (mouseRightReleased)
            {
                movementEnabled = false;

                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            movementEnabled = movementEnabled;

            if (movementEnabled && ImGui::IsWindowFocused())
            {
                // Apply mouse rotation deltas
                camera.rotation.y += -mouseDelta.y * sensitivity; // up/down
                camera.rotation.x += -mouseDelta.x * sensitivity; // left/right

                // Clamp vertical rotation to prevent flipping
                camera.rotation.y = glm::clamp(camera.rotation.y, -89.99f, 89.99f);

                // Build rotation quaternion from yaw (around Y) and pitch (around X)
                glm::quat qPitch = glm::angleAxis(glm::radians(camera.rotation.y), glm::vec3(1, 0, 0));
                glm::quat qYaw = glm::angleAxis(glm::radians(camera.rotation.x), glm::vec3(0, 1, 0));
                cameraTransform.local.rotation = qYaw * qPitch; // yaw first, then pitch

                // Compute direction vectors
                camera.back = cameraTransform.local.rotation * glm::vec3(0, 0, 1);
                camera.right = cameraTransform.local.rotation * glm::vec3(1, 0, 0);
                camera.up = cameraTransform.local.rotation * glm::vec3(0, 1, 0); // local up

                const glm::vec3 forward = glm::normalize(qYaw * glm::vec3(0, 0, -1));
                const glm::vec3 left = glm::normalize(qYaw * glm::vec3(-1, 0, 0));
                const glm::vec3 up = glm::vec3(0, 1, 0);

                // Speed boost
                float moveSpeed = movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                    moveSpeed *= boostMuliplier;

                // Movement
                if (glfwGetKey(window, GLFW_KEY_W))
                    cameraTransform.local.position += forward * moveSpeed;
                if (glfwGetKey(window, GLFW_KEY_S))
                    cameraTransform.local.position -= forward * moveSpeed;
                if (glfwGetKey(window, GLFW_KEY_A))
                    cameraTransform.local.position += left * moveSpeed;
                if (glfwGetKey(window, GLFW_KEY_D))
                    cameraTransform.local.position -= left * moveSpeed;
                if (glfwGetKey(window, GLFW_KEY_E))
                    cameraTransform.local.position += up * moveSpeed;
                if (glfwGetKey(window, GLFW_KEY_Q))
                    cameraTransform.local.position -= up * moveSpeed;
            }

            ImVec2 contentRegionSize = ImGui::GetContentRegionAvail();
            ImVec2 contentRegionPos = ImGui::GetCursorScreenPos();

            camera.renderTarget.Resize({ contentRegionSize.x,contentRegionSize.y });

            // draw to everything to the imgui texture
            ImGui::Image(camera.renderTarget.GetTexture(0), contentRegionSize, ImVec2(0, 1), ImVec2(1, 0)); // flipped uvs

            // if movement is enabled do not do any gizmos
            if (movementEnabled || !ImGui::IsWindowFocused())
            {
                ImGui::End();
                return;
            }

            static bool guizmoActive = false;
            static ImGuizmo::OPERATION currentOperation = ImGuizmo::OPERATION::TRANSLATE;
            static ImGuizmo::MODE currentMode = ImGuizmo::MODE::WORLD;

            // maya keybinds for changing the current gizmo
            if (ImGui::IsKeyDown(ImGuiKey_W))
            {
                currentOperation = ImGuizmo::OPERATION::TRANSLATE;
                currentMode = ImGuizmo::MODE::WORLD;
            }
            else if (ImGui::IsKeyDown(ImGuiKey_E))
            {
                currentOperation = ImGuizmo::OPERATION::ROTATE;
                currentMode = ImGuizmo::MODE::WORLD;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_R) && !ImGui::GetIO().KeyCtrl)
            {
                Gep::Log::Error("Scale is not implemented");

                //currentOperation = ImGuizmo::OPERATION::SCALE;
                //currentMode = ImGuizmo::MODE::WORLD;
            }

            // prepare gizmos for rendering
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(contentRegionPos.x, contentRegionPos.y, contentRegionSize.x, contentRegionSize.y);
            ImGuizmo::SetOrthographic(false);
            glm::mat4 view = camera.GetViewMatrix(cameraTransform.world.position);
            glm::mat4 pers = camera.GetProjectionMatrix();

            struct EntityTransformPair
            {
                Gep::Entity entity;
                Client::Transform& transform;
            };

            const auto& selectedEntities = mEditorResource.GetSelectedEntities();
            std::vector<EntityTransformPair> selectedWithTransform;
            glm::vec3 avgPos(0.0f);
            size_t avgCount = 0;
            auto hasSelectedAncestor = [&](Gep::Entity entity) -> bool
            {
                while (mManager.HasParent(entity))
                {
                    Gep::Entity parent = mManager.GetParent(entity);
                    if (selectedEntities.find(parent) != selectedEntities.end())
                    {
                        return true;
                    }

                    entity = parent;
                }

                return false;
            };


            // get the averages of all selected entities
            for (Gep::Entity e : selectedEntities)
            {
                if (mManager.HasComponent<Client::Transform>(e))
                {
                    auto& tf = mManager.GetComponent<Client::Transform>(e);
                    avgPos += tf.world.position;
                    avgCount++;
                    if (hasSelectedAncestor(e))
                        continue; // skip entities who's ancestors are also selected

                    selectedWithTransform.emplace_back(e, tf);
                }
            }

            // if any of the selected entities had a transform get the average model matrix
            if (!selectedWithTransform.empty())
            {
                avgPos /= avgCount;

                glm::mat4 gizmoTransform = glm::translate(glm::mat4(1.0f), avgPos);
                glm::mat4 deltaMatrix(1.0f);
                constexpr float snap[3] = { 0.1f, 0.1f, 0.1f };

                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(pers),
                    currentOperation,
                    currentMode,
                    glm::value_ptr(gizmoTransform),
                    glm::value_ptr(deltaMatrix),
                    snap))
                {
                    for (auto& [e, tf] : selectedWithTransform)
                    {
                        // Get parent's world transform
                        Gep::VQS parentWorld{};
                        if (mManager.HasParent(e) && mManager.HasComponent<Client::Transform>(mManager.GetParent(e)))
                        {
                            auto& parentTf = mManager.GetComponent<Client::Transform>(mManager.GetParent(e));
                            parentWorld = parentTf.world;
                        }

                        // Apply gizmo delta to *world matrix*
                        tf.world = Gep::ToVQS(deltaMatrix) * tf.world;

                        // Recalculate local
                        tf.local = Gep::Inverse(parentWorld) * tf.world;
                    }
                }
                guizmoActive = true;
            }
            else
                guizmoActive = false;

            // when the window Is clicked set the focus and fire a ray
            if (ImGui::IsKeyPressed(ImGuiKey_F) && !selectedWithTransform.empty())
            {
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
            {
                ImGui::SetWindowFocus();

                ImVec2 mousePos = ImGui::GetMousePos();

                // create a ray based on the content region
                Gep::Ray ray = Gep::Ray::FromMouse(
                    glm::vec2(mousePos.x - contentRegionPos.x, mousePos.y - contentRegionPos.y),
                    glm::vec2(contentRegionSize.x, contentRegionSize.y),
                    cameraTransform.world.position,
                    camera.GetViewMatrix(cameraTransform.world.position),
                    camera.GetProjectionMatrix()
                );

                std::vector<Gep::Entity> hitEntities = mCollisionResource.RayCast(mManager, ray);

                // if the gizmo is not in use or if the gizmo is not enabled (for some reason IsOver returns true when its not active)
                if (!ImGuizmo::IsOver() || !guizmoActive)
                {
                    if (!hitEntities.empty())
                        mEditorResource.SmartSelectEntity(hitEntities.front(), window);
                    else if (!glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                        mEditorResource.DeselectAll();
                }
            }
        }

        ImGui::End();
        
    }

    void RenderSystem::AddColliders()
    {
        mManager.ForEachArchetype([&](Gep::Entity entity, CubeCollider& collider, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);

            Gep::StaticObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2]
            };

            mRenderer.AddStaticObject("PBR-Static", "Cube", uniforms, Gep::RenderFlags::Wireframe);
        });

        mManager.ForEachArchetype([&](Gep::Entity entity, SphereCollider& collider, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);

            Gep::StaticObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2]
            };

            mRenderer.AddStaticObject("PBR-Static", "Sphere", uniforms, Gep::RenderFlags::Wireframe);
        });
    }

    void RenderSystem::AddLights()
    {
        // prepares all of the light uniform values
        mManager.ForEachArchetype([&](Gep::Entity e, Light& l, Transform& t)
        {
            if (mManager.HasComponent<ShadowCasterComponent>(e))
                return; // skip lights that also have shadow caster components, they will be added in the next loop

            if (l.intensity <= 0.0f)
                return; // skip lights that have no intensity
            if (!l.enabled)
                return; // skip disabled lights

            float cutoff = 0.1f;              // chosen threshold
            float radius = std::sqrt(l.intensity / cutoff);

            glm::mat4 modelMatrix{ 1.0f };
            modelMatrix = glm::translate(modelMatrix, t.world.position);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(radius));

            Gep::PointLightGPUData uniforms
            {
                .position = t.world.position,
                .color = l.color,
                .intensity = l.intensity,

                .modelMatrix = modelMatrix
            };

            mRenderer.AddPointLight(uniforms);
        });

        // prepares all of the light uniform values
        mManager.ForEachArchetype([&](Gep::Entity e, Light& l, ShadowCasterComponent& sc, Transform& t)
        {
            if (l.intensity <= 0.0f)
                return; // skip lights that have no intensity
            if (!l.enabled)
                return; // skip disabled lights

            float cutoff = 0.1f;              // chosen threshold
            float radius = std::sqrt(l.intensity / cutoff);

            glm::mat4 modelMatrix{ 1.0f };
            modelMatrix = glm::translate(modelMatrix, t.world.position);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(radius));

            float nearPlane = 1.0f;
            float farPlane = radius;
            float aspectRatio = (float)sc.shadowMap.GetSize().x / (float)sc.shadowMap.GetSize().y;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspectRatio, nearPlane, farPlane);

            glm::mat4 shadow0 = shadowProj * glm::lookAt(t.world.position, t.world.position + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
            glm::mat4 shadow1 = shadowProj * glm::lookAt(t.world.position, t.world.position + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
            glm::mat4 shadow2 = shadowProj * glm::lookAt(t.world.position, t.world.position + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
            glm::mat4 shadow3 = shadowProj * glm::lookAt(t.world.position, t.world.position + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
            glm::mat4 shadow4 = shadowProj * glm::lookAt(t.world.position, t.world.position + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
            glm::mat4 shadow5 = shadowProj * glm::lookAt(t.world.position, t.world.position + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));

            GLuint64 cubemapHandle = sc.shadowMap.GetTextureAttachments().at(0).handle;

            Gep::PointLightGPUData light
            {
                .position = t.world.position,
                .color = l.color,
                .intensity = l.intensity,

                .modelMatrix = modelMatrix,
            };

            Gep::PointLightShadowGPUData uniforms
            {
                .light = light,
                .shadowMatrices = { shadow0, shadow1, shadow2, shadow3, shadow4, shadow5 },
                .shadowMapHandle = cubemapHandle
            };
            
            mRenderer.AddPointLightShadow(uniforms, sc.shadowMap);
        });

        // prepares all of the directional light uniform values
        mManager.ForEachArchetype([&](Gep::Entity e, DirectionalLight& l, Transform& t)
        {
            if (mManager.HasComponent<ShadowCasterDirectionalComponent>(e))
                return; // skip lights that also have shadow caster components, they will be added in the next loop

            if (l.intensity <= 0.0f)
                return; // skip lights that have no intensity
            if (!l.enabled)
                return; // skip disabled lights

            Gep::DirectionalLightGPUData uniforms
            {
                .position = t.world.position,
                .color = l.color,
                .intensity = l.intensity,
                .direction = t.world.rotation * glm::vec3(0, 1, 0)
            };

            mRenderer.AddDirectionalLight(uniforms);
        });

        // prepares all shadowing directional light uniform values
        mManager.ForEachArchetype([&](Gep::Entity e, DirectionalLight& l, ShadowCasterDirectionalComponent& sc, Transform& t)
        {
            if (l.intensity <= 0.0f)
                return; // skip lights that have no intensity
            if (!l.enabled)
                return; // skip disabled lights

            float near = 1.0f;
            float far = t.world.scale.z;

            const glm::vec3 lightDirection = glm::normalize(t.world.rotation * glm::vec3(0, 1, 0));
            const glm::vec3 up = std::abs(glm::dot(lightDirection, glm::vec3(0, 1, 0))) > 0.99f
                ? glm::vec3(0, 0, 1)
                : glm::vec3(0, 1, 0);

            glm::vec3 shadowCamPos = t.world.position - lightDirection * far * 0.5f;
            glm::mat4 lightProj = glm::ortho(-t.world.scale.x * 0.5f, t.world.scale.x * 0.5f, -t.world.scale.y * 0.5f, t.world.scale.y * 0.5f, near, far);
            glm::mat4 lightView = glm::lookAt(shadowCamPos, t.world.position, up);

            GLuint64 shadowMapHandle = sc.shadowMap.GetTextureAttachments().at(0).handle;

            Gep::DirectionalLightGPUData light
            {
                .position = t.world.position,
                .color = l.color,
                .intensity = l.intensity,
                .direction = lightDirection
            };

            Gep::DirectionalLightShadowGPUData uniforms
            {
                .light = light,

                .pvMatrix = lightProj * lightView,
                .shadowMapHandle = shadowMapHandle,
            };

            mRenderer.AddDirectionalLightShadow(uniforms, sc.shadowMap);
        });
    }

    void RenderSystem::AddCameras()
    {
        // prepares the camera uniforms
        mManager.ForEachArchetype([&](Gep::Entity camEntity, Transform& camTransform, Camera& cam)
        {
            const glm::mat4 pvMatrix = cam.GetProjectionMatrix() * cam.GetViewMatrix(camTransform.world.position);
            const Gep::CameraGPUData uniforms
            {
                .pvMatrix = pvMatrix,
                .ipvMatrix = glm::inverse(pvMatrix),
                .position = camTransform.world.position
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

        mManager.ForEachArchetype([&](Gep::Entity entity, RiggedModelComponent& model, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);
            const Gep::Model& internalModel = mRenderer.GetModel(model.name);

            std::string targetShader = "PBR-Static";

            // if the model also has an animation compute its final pose and pass all bone info to the gpu
            int previousBoneOffset = boneOffset;
            bool hasRealBones = false;

            for (uint32_t i = 0; i < model.pose.size() && i < internalModel.skeleton.bones.size(); ++i)
            {
                const Gep::Bone& b = internalModel.skeleton.bones[i];

                // if any bone is real switch to skinned shader
                if (b.isRealBone)
                    hasRealBones = true;

                Gep::BoneGPUData boneData{
                    .offsetMatrix = Gep::ToMat4(model.pose[i] * b.inverseBind)
                };

                mRenderer.AddBone(boneData);
                ++boneOffset;
            }

            if (hasRealBones)
                targetShader = "PBR-Skinned";


            if (mDrawBones)
            {
                DrawSkeleton(internalModel.skeleton, modelMatrix, model.pose, skeletonLines);
            }


            Gep::StaticObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2],

                .boneOffset = previousBoneOffset // only used in the PBR-Skinned shader
            };

            Gep::RenderFlags flags = Gep::RenderFlags::None;
            if (mWireframeMode)
                flags |= Gep::RenderFlags::Wireframe;

            if (model.selected)
                flags |= Gep::RenderFlags::Highlight;

            mRenderer.AddStaticObject(targetShader, model.name, uniforms, flags);
        });

        mManager.ForEachArchetype([&](Gep::Entity entity, StaticModelComponent& model, Transform& transform)
        {
            const glm::mat4 modelMatrix = Gep::ToMat4(transform.world);
            const glm::mat3 normal = Gep::NormalFromModel(modelMatrix);
            const Gep::Model& internalModel = mRenderer.GetModel(model.name);

            Gep::StaticObjectGPUData uniforms
            {
                .modelMatrix = modelMatrix,
                .normalMatrixCol0 = normal[0],
                .normalMatrixCol1 = normal[1],
                .normalMatrixCol2 = normal[2],

                .boneOffset = 0 // this is not used when using the static shader
            };

            Gep::RenderFlags flags = Gep::RenderFlags::None;
            if (mWireframeMode)
                flags |= Gep::RenderFlags::Wireframe;

            if (model.selected)
                flags |= Gep::RenderFlags::Highlight;

            mRenderer.AddStaticObject("PBR-Static", model.name, uniforms, flags);
        });

        mRenderer.AddLine(skeletonLines);

    }

    void RenderSystem::InitializeModelPose(RiggedModelComponent& modelComponent, const Gep::Model& internalModel)
    {
        modelComponent.pose.clear();
        modelComponent.pose.resize(internalModel.skeleton.bones.size());

        // initialize with the default skeleton
        for (size_t i = 0; i < modelComponent.pose.size(); ++i)
        {
            modelComponent.pose[i] = internalModel.skeleton.bones[i].transformation; // at this point .pose is in local space
        }

        // calculate the global pose
        for (uint32_t i = 1; i < internalModel.skeleton.bones.size(); ++i) // note skip the root bone
        {
            uint32_t parent = internalModel.skeleton.bones[i].parentIndex;

            modelComponent.pose[i] = modelComponent.pose[parent] * modelComponent.pose[i];
        }
    }
}


