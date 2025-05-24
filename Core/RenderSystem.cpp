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
#include "Material.hpp"
#include "CameraComponent.hpp"
#include "TextureComponent.hpp"
#include "LightComponent.hpp"
#include "SkyboxMesh.hpp"


namespace Client
{
    RenderSystem::RenderSystem(Gep::EngineManager& em)
        : ISystem(em)
    {

    }

    RenderSystem::~RenderSystem()
    {
    }

    void RenderSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Mesh>>(this, &RenderSystem::OnModelAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Mesh>>(this, &RenderSystem::OnMeshEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Texture>>(this, &RenderSystem::OnTextureEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Light>>(this, &RenderSystem::OnLightEditorRender);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Camera>>(this, &RenderSystem::OnCameraEditorRender);

        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        renderer.LoadVertexShader("assets\\shaders\\Lighting.vert");
        renderer.LoadFragmentShader("assets\\shaders\\Lighting.frag");

        renderer.Compile();
        renderer.BackfaceCull();
        renderer.SetAmbientLight({ 0.2f, 0.2f, 0.2f });

        renderer.LoadErrorTexture("assets\\textures\\Error.png");

        renderer.LoadMesh("Quad", Gep::QuadMesh());
        renderer.LoadMesh("Sphere", Gep::SphereMesh(10, 10));
        renderer.LoadMesh("Cube", Gep::CubeMesh());
        renderer.LoadMesh("Icosphere", Gep::IcosphereMesh(3));
        renderer.LoadMesh("Skybox", Gep::SkyboxMesh());
    }

    void RenderSystem::Update(float dt)
    {
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        renderer.Start();

        mManager.ForEachArchetype<Light, Transform>([&](Gep::Entity e, Light& l, Transform& t)
        {
            renderer.AddLight(l.color, t.position, l.intensity);
        });

        renderer.DrawLights();

        // begins rendering each everything
        mManager.ForEachArchetype<Transform, Camera>([&](Gep::Entity camEntity, Transform& camTransform, Camera& cam)
        {
            cam.renderTarget->Bind();
            cam.renderTarget->Clear({ 0.1f, 0.1f, 0.1f });

            // convert the camera's rotation to radians
            glm::vec3 camRotation = glm::radians(camTransform.rotation);

            // calculate the camera's right, up, and back vectors from the transforms rotation
            cam.right = { cos(camRotation.y), 0, sin(camRotation.y) };
            cam.up = { sin(camRotation.x) * sin(camRotation.y), cos(camRotation.x), -sin(camRotation.x) * cos(camRotation.y) };
            cam.back = glm::normalize(glm::cross(cam.right, cam.up));

            const glm::mat4 pers = cam.GetProjectionMatrix();
            const glm::mat4 view = cam.GetViewMatrix(camTransform.position);

            renderer.SetCamera(pers, view, camTransform.position);
            const glm::vec2 renderSize = cam.renderTarget->GetSize();

            mManager.ForEachArchetype<Mesh, Transform>([&](Gep::Entity entity, Mesh& material, Transform& transform)
            {
                glm::mat4 model = Gep::translation_matrix(transform.position)
                    * Gep::rotation(transform.rotation)
                    * Gep::scale_matrix(transform.scale);

                renderer.SetModel(model);
                renderer.SetMaterial(material.color, material.spec_coeff, material.spec_exponent);

                if (mManager.HasComponent<Texture>(entity))
                {
                    const Texture& texture = mManager.GetComponent<Texture>(entity);
                    GLuint textureid = renderer.GetOrLoadTexture(texture.texturePath);
                    renderer.SetTexture(textureid);
                }

                renderer.SetHighlight(material.selected);

                if (mManager.HasComponent<Client::Light>(entity))
                {
                    Light& light = mManager.GetComponent<Client::Light>(entity);
                    renderer.SetSolidColor(light.color * light.intensity);
                }

                renderer.SetIgnoreLight(material.ignoreLight);

                uint64_t meshID = renderer.GetOrLoadMesh(material.meshName);
                renderer.DrawMesh(meshID);

                //ImGuizmo::SetDrawlist();
                //ImGuizmo::SetRect(cam.renderTarget->GetPosition().x, cam.renderTarget->GetPosition().y, renderSize.x, renderSize.y);
                //ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(pers), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, glm::value_ptr(model));
            });

            if (mDrawColliders)
            {

                mManager.ForEachArchetype<SphereCollider, Transform>([&](Gep::Entity e, SphereCollider& collider, Transform& transform)
                {
                    glm::mat4 model = Gep::translation_matrix(transform.position)
                        * Gep::rotation(transform.rotation)
                        * Gep::scale_matrix(std::max({ transform.scale.x, transform.scale.y, transform.scale.z }));

                    renderer.SetModel(model);
                    renderer.SetWireframe(true);
                    renderer.SetSolidColor({ 1.0f, 0.0f, 0.0f });

                    uint64_t meshID = renderer.GetMesh("Icosphere");
                    renderer.DrawMesh(meshID);
                });

                mManager.ForEachArchetype<CubeCollider, Transform>([&](Gep::Entity e, CubeCollider& collider, Transform& transform)
                {
                    glm::mat4 model = Gep::translation_matrix(transform.position)
                        * Gep::rotation(transform.rotation)
                        * Gep::scale_matrix(transform.scale);

                    renderer.SetModel(model);
                    renderer.SetWireframe(true);
                    renderer.SetSolidColor({ 1.0f, 0.0f, 0.0f });

                    uint64_t meshID = renderer.GetMesh("Cube");
                    renderer.DrawMesh(meshID);
                });
            }

            cam.renderTarget->Draw(mManager, camEntity);
            cam.Resize(renderSize);
            cam.renderTarget->Unbind();
        });

        mManager.ForEachArchetype<Mesh, Transform>([&](Gep::Entity entity, Mesh& material, Transform& transform)
        {
            material.selected = false;
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
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        const float movementSpeed = 10 * dt;

        GLFWwindow* window = glfwGetCurrentContext();

        static bool isTKeyPressed = false;
        static bool isYKeyPressed = false;
        static bool isUKeyPressed = false;

        // Handle T key for textures
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
        {
            if (!isTKeyPressed)
            {
                renderer.ToggleTextures();
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
                renderer.ToggleWireframes();
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
    }

    void RenderSystem::RenderImGui(float dt)
    {
        ImGui::Begin("Render System");
        ImGui::End();
    }
    void RenderSystem::OnModelAdded(const Gep::Event::ComponentAdded<Mesh>& event)
    {
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        if (!mManager.HasComponent<Transform>(event.entity)
            || !mManager.HasComponent<Mesh>(event.entity))
        {
            return;
        }

        Mesh& material = mManager.GetComponent<Mesh>(event.entity);
        Transform& transform = mManager.GetComponent<Transform>(event.entity);


        //renderer.mBVHTree.insert(event.entity, material.meshName);
    }

    void RenderSystem::OnMeshEditorRender(const Gep::Event::ComponentEditorRender<Mesh>& event)
    {
        Mesh& mesh = event.component;

        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();
        Client::EditorResource& er = mManager.GetResource<Client::EditorResource>();
        std::vector<std::string> loadedMeshes = renderer.GetLoadedMeshes();

        // drop down for selecting a mesh
        bool meshesOpen = ImGui::BeginCombo("Meshs", mesh.meshName.c_str());

        const std::vector<std::string>& allowedExtensions = renderer.GetSupportedModelFormats();

        er.AssetBrowserDropTarget(allowedExtensions, [&](const std::filesystem::path& droppedPath)
        {
            if (!renderer.IsMeshLoaded(droppedPath.string()))
            {
                renderer.LoadMesh(droppedPath);
            }

            mesh.meshName = droppedPath.string();
        });

        if (meshesOpen)
        {
            for (const std::string& meshName : loadedMeshes)
            {
                bool isSelected = (meshName == mesh.meshName);
                if (ImGui::Selectable(meshName.c_str(), isSelected))
                {
                    mesh.meshName = meshName;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::ColorEdit3("Color", &mesh.color[0]);
        ImGui::ColorEdit3("Specular Color", &mesh.spec_coeff[0]);
        ImGui::DragFloat("Specular Exponent", &mesh.spec_exponent, 0.1f, 0.1f, FLT_MAX);
        ImGui::Checkbox("Ignore Light", &mesh.ignoreLight);
    }

    void RenderSystem::OnTextureEditorRender(const Gep::Event::ComponentEditorRender<Texture>& event)
    {
        Texture& texture = event.component;

        const Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();
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
        ImGui::DragFloat("Range", &light.intensity, 1.0f, 0.001f, Gep::num_max<float>());
    }

    void RenderSystem::OnCameraEditorRender(const Gep::Event::ComponentEditorRender<Camera>& event)
    {
        Camera& camera = event.component;

        ImGui::DragFloat("near plane", &camera.nearPlane, 0.1f, 0.001f, 10000.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("far plane", &camera.farPlane, 0.1f, 0.001f, 10000.0f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
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


