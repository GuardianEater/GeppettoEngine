/*****************************************************************//**
 * \file   RenderSystem.cpp
 * \brief  System that renders objects
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "pch.hpp"

#include "RenderSystem.hpp"

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
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        renderer.LoadVertexShader("assets\\shaders\\Lighting.vert");
        renderer.LoadFragmentShader("assets\\shaders\\Lighting.frag");
        renderer.Compile();

        renderer.BackfaceCull();
        renderer.SetAmbientLight({ 0.1, 0.1, 0.1 });

        renderer.LoadImage("Fox", "assets\\textures\\Fox.jpg");
        renderer.LoadImage("Raccoon", "assets\\textures\\Raccoon.jpg");
        renderer.LoadImage("Kurisu", "assets\\textures\\Kurisu.png");
        renderer.LoadImage("Checker", "assets\\textures\\Checker.jpg");
        renderer.LoadImage("Okayu1", "assets\\textures\\Okayu1.jpg");
        renderer.LoadImage("Okayu2", "assets\\textures\\Okayu2.PNG");
        renderer.LoadImage("Peko", "assets\\textures\\Peko.jpg");

        renderer.LoadMesh("Quad", Gep::QuadMesh());
        renderer.LoadMesh("Sphere", Gep::SphereMesh(10, 10));
        renderer.LoadMesh("Cube", Gep::CubeMesh());
        renderer.LoadMesh("Icosphere", Gep::IcosphereMesh(3));

        //mRenderer.CreateLight(0, { 0, 10, 0 }, { 1, 0, 0 });
        mManager.SubscribeToEvent<Gep::Event::WindowResize>(this, &Client::RenderSystem::WindowResizeEvent);
        mManager.SubscribeToEvent<Gep::Event::MouseMoved>(this, &Client::RenderSystem::MouseMovedEvent);
        mManager.SubscribeToEvent<Gep::Event::MouseClicked>(this, &Client::RenderSystem::MouseClickedEvent);
        mManager.SubscribeToEvent<Gep::Event::KeyPressed>(this, &Client::RenderSystem::KeyPressedEvent);
        mManager.SubscribeToEvent<Gep::Event::MouseScrolled>(this, &Client::RenderSystem::MouseScrolledEvent);
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
            cam.renderTarget->Clear({0.1f, 0.1f, 0.1f});

            // convert the camera's rotation to radians
            glm::vec3 camRotation = glm::radians(camTransform.rotation);

            // calculate the camera's right, up, and back vectors from the transforms rotation
            cam.right = { cos(camRotation.y), 0, sin(camRotation.y) };
            cam.up = { sin(camRotation.x) * sin(camRotation.y), cos(camRotation.x), -sin(camRotation.x) * cos(camRotation.y) };
            cam.back = glm::normalize(glm::cross(cam.right, cam.up));

            const glm::mat4 pers = cam.GetProjectionMatrix();
            const glm::mat4 view = cam.GetViewMatrix(camTransform.position);

            renderer.SetCamera(pers, view, camTransform.position);

            const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform, Material>();
            for (Gep::Entity entity : entities)
            {
                const Transform& transform = mManager.GetComponent<Transform>(entity);
                Material& material = mManager.GetComponent<Material>(entity);

                const glm::mat4 model = Gep::translation_matrix(transform.position)
                                      * Gep::rotation(-transform.rotation)
                                      * Gep::scale_matrix(transform.scale);

                renderer.SetModel(model);
                renderer.SetMaterial(material.diff_coeff, material.spec_coeff, material.spec_exponent);
                
                if (mManager.HasComponent<Texture>(entity))
                {
                    const Texture& texture = mManager.GetComponent<Texture>(entity);
                    renderer.SetTexture(texture.textureName);
                }

                renderer.SetHighlight(material.selected);

                if (mManager.HasComponent<Client::Light>(entity))
                {
                    Light& light = mManager.GetComponent<Client::Light>(entity);
                    renderer.SetSolidColor(light.color * light.intensity);
                }

                renderer.DrawMesh(material.meshName);
                material.selected = false;
            }

            cam.renderTarget->Draw(mManager, cameraEntity);
            cam.Resize(cam.renderTarget->GetSize());
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

        // Handle T key for textures
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            if (!isTKeyPressed) { // Key was just pressed
                renderer.ToggleTextures();
                isTKeyPressed = true;
            }
        }
        else {
            isTKeyPressed = false; // Reset when key is released
        }

        // Handle Y key for wireframes
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            if (!isYKeyPressed) { // Key was just pressed
                renderer.ToggleWireframes();
                isYKeyPressed = true;
            }
        }
        else {
            isYKeyPressed = false; // Reset when key is released
        }
    }

    void RenderSystem::RenderImGui(float dt)
    {
        ImGui::Begin("Render System");
        ImGui::End();
    }

    void RenderSystem::WindowResizeEvent(const Gep::Event::WindowResize& eventData)
    {
        //glViewport(0, 0, eventData.width, eventData.height);

        //const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        //for (Gep::Entity cameraEntity : cameras)
        //{
        //    // update the aspect ratio of the camera
        //    Camera& cam = mManager.GetComponent<Camera>(cameraEntity);
        //    cam.viewport.y = cam.viewport.x / eventData.width * eventData.height;
        //    cam.viewport.z = cam.nearPlane;
        //    cam.viewport.x = 2.0f * cam.nearPlane * glm::tan(glm::radians(80.0f / 2.0f));
        //}
    }

    void RenderSystem::MouseMovedEvent(const Gep::Event::MouseMoved& eventData)
    {
        //// only while right mouse button is pressed
        //if (glfwGetMouseButton(glfwGetCurrentContext(), GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) return;


        //float sensitivity = 0.3f;

        //const double dx = (eventData.x - eventData.prevX) * sensitivity;
        //const double dy = (eventData.y - eventData.prevY) * sensitivity;

        //const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();

        //for (Gep::Entity cameraEntity : cameras)
        //{
        //    Transform& camTransform = mManager.GetComponent<Transform>(cameraEntity);
        //    Camera& cam = mManager.GetComponent<Camera>(cameraEntity);
        //    camTransform.rotation.y += dx;
        //    camTransform.rotation.x += dy;
        //    if (camTransform.rotation.x > 89.0f) camTransform.rotation.x = 89.0f;
        //    if (camTransform.rotation.x < -89.0f) camTransform.rotation.x = -89.0f;
        //}
    }

    void RenderSystem::MouseClickedEvent(const Gep::Event::MouseClicked& eventData)
    {
        //if (eventData.action == GLFW_PRESS && eventData.button == GLFW_MOUSE_BUTTON_RIGHT)
        //{
        //    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        //}
        //else if (eventData.action == GLFW_RELEASE && eventData.button == GLFW_MOUSE_BUTTON_RIGHT)
        //{
        //    glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        //}
    }

    void RenderSystem::KeyPressedEvent(const Gep::Event::KeyPressed& eventData)
    {

    }

    void RenderSystem::MouseScrolledEvent(const Gep::Event::MouseScrolled& eventData)
    {
        //const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        //for (Gep::Entity cameraEntity : cameras)
        //{
        //    Camera& cam = mManager.GetComponent<Camera>(cameraEntity);
        //    Transform& camTransform = mManager.GetComponent<Transform>(cameraEntity);
        //    camTransform.position += cam.back * -(float)eventData.yoffset;
        //}
    }
}


