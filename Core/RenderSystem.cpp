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
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Material>>(this, &RenderSystem::OnModelAdded);

        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        renderer.LoadVertexShader("assets\\shaders\\Lighting.vert");
        renderer.LoadFragmentShader("assets\\shaders\\Lighting.frag");

        renderer.Compile();
        renderer.BackfaceCull();
        renderer.SetAmbientLight({ 0.1, 0.1, 0.1 });

        renderer.LoadErrorTexture("assets\\textures\\Error.png");

        renderer.LoadMesh("Quad", Gep::QuadMesh());
        renderer.LoadMesh("Sphere", Gep::SphereMesh(10, 10));
        renderer.LoadMesh("Cube", Gep::CubeMesh());
        renderer.LoadMesh("Icosphere", Gep::IcosphereMesh(3));
        renderer.LoadMesh("assets\\meshes\\dragon.obj");
        renderer.LoadMesh("assets\\meshes\\neko.obj");
        renderer.LoadMesh("assets\\meshes\\mesh6.obj");
        renderer.LoadMesh("assets\\meshes\\cube.obj");
    }

    void RenderSystem::Update(float dt)
    {
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        renderer.Start();

        // adds the lights to the scene
        const std::vector<Gep::Entity>& lights = mManager.GetEntities<Light, Transform>();
        for (Gep::Entity lightEntity : lights)
        {
            const Light& light = mManager.GetComponent<Light>(lightEntity);
            const Transform& lightTransform = mManager.GetComponent<Transform>(lightEntity);

            renderer.AddLight(light.color, lightTransform.position, light.intensity);
        }

        renderer.DrawLights();

        // begins rendering each everything
        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        for (Gep::Entity cameraEntity : cameras)
        {
            const Transform& camTransform = mManager.GetComponent<Transform>(cameraEntity);
            Camera& cam = mManager.GetComponent<Camera>(cameraEntity);

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

            const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform, Material>();
            for (Gep::Entity entity : entities)
            {
                const Transform& transform = mManager.GetComponent<Transform>(entity);
                Material& material = mManager.GetComponent<Material>(entity);

                glm::mat4 model = Gep::translation_matrix(transform.position)
                    * Gep::rotation(-transform.rotation)
                    * Gep::scale_matrix(transform.scale);

                renderer.SetModel(model);
                renderer.SetMaterial(material.diff_coeff, material.spec_coeff, material.spec_exponent);

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

                uint64_t meshID = renderer.GetMesh(material.meshName);
                renderer.DrawMesh(meshID);
                material.selected = false;

                //ImGuizmo::SetDrawlist();
                //ImGuizmo::SetRect(cam.renderTarget->GetPosition().x, cam.renderTarget->GetPosition().y, renderSize.x, renderSize.y);
                //ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(pers), ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, glm::value_ptr(model));
            }

            if (mDrawColliders)
            {
                const std::vector<Gep::Entity>& sphereColliders = mManager.GetEntities<Transform, SphereCollider>();
                for (Gep::Entity entity : sphereColliders)
                {
                    const Transform& transform = mManager.GetComponent<Transform>(entity);
                    const SphereCollider& collider = mManager.GetComponent<SphereCollider>(entity);
                    glm::mat4 model = Gep::translation_matrix(transform.position)
                        * Gep::rotation(-transform.rotation)
                        * Gep::scale_matrix(std::max({ transform.scale.x, transform.scale.y, transform.scale.z }));
                    renderer.SetModel(model);
                    renderer.SetWireframe(true);
                    //renderer.SetBackfaceCull(false);
                    renderer.SetSolidColor({ 1.0f, 0.0f, 0.0f });
                    uint64_t meshID = renderer.GetMesh("Icosphere");
                    renderer.DrawMesh(meshID);
                }

                const std::vector<Gep::Entity>& cubeColliders = mManager.GetEntities<Transform, CubeCollider>();
                for (Gep::Entity entity : cubeColliders)
                {
                    const Transform& transform = mManager.GetComponent<Transform>(entity);
                    const CubeCollider& collider = mManager.GetComponent<CubeCollider>(entity);
                    glm::mat4 model = Gep::translation_matrix(transform.position)
                        * Gep::rotation(-transform.rotation)
                        * Gep::scale_matrix(transform.scale);
                    renderer.SetModel(model);
                    renderer.SetWireframe(true);
                    //renderer.SetBackfaceCull(false);
                    renderer.SetSolidColor({ 1.0f, 0.0f, 0.0f });
                    uint64_t meshID = renderer.GetMesh("Cube");
                    renderer.DrawMesh(meshID);
                }
            }

            cam.renderTarget->Draw(mManager, cameraEntity);
            cam.Resize(renderSize);
            cam.renderTarget->Unbind();
        }



        renderer.End();
        HandleInputs(dt);
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
    void RenderSystem::OnModelAdded(const Gep::Event::ComponentAdded<Material>& event)
    {
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        if (!mManager.HasComponent<Transform>(event.entity)
            || !mManager.HasComponent<Material>(event.entity))
        {
            return;
        }

        Material& material = mManager.GetComponent<Material>(event.entity);
        Transform& transform = mManager.GetComponent<Transform>(event.entity);


        //renderer.mBVHTree.insert(event.entity, material.meshName);
    }
}


