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
        , mRenderer()
    {
        mRenderer.LoadVertexShader("assets\\shaders\\FlatShading.vert");
        mRenderer.LoadFragmentShader("assets\\shaders\\FlatShading.frag");
        mRenderer.Compile();

        mRenderer.BackfaceCull();
        mRenderer.SetAmbientLight({ 0.2, 0.2, 0.2 });

        mRenderer.LoadImage("Fox", "assets\\textures\\Fox.jpg");
        mRenderer.LoadImage("Raccoon", "assets\\textures\\Raccoon.jpg");
        mRenderer.LoadImage("Kurisu", "assets\\textures\\Kurisu.png");
        mRenderer.LoadImage("Checker", "assets\\textures\\Checker.jpg");
        mRenderer.LoadImage("Okayu1", "assets\\textures\\Okayu1.jpg");
        mRenderer.LoadImage("Okayu2", "assets\\textures\\Okayu2.PNG");
        mRenderer.LoadImage("Peko", "assets\\textures\\Peko.jpg");

        mRenderer.LoadMesh("Quad", Gep::QuadMesh());
        mRenderer.LoadMesh("Sphere", Gep::SphereMesh(10, 10));
        mRenderer.LoadMesh("Cube", Gep::CubeMesh());
        mRenderer.LoadMesh("Icosphere", Gep::IcosphereMesh(3));
    }

    RenderSystem::~RenderSystem()
    {
        mRenderer.UnloadMesh("Sphere");
        mRenderer.UnloadMesh("Cube");
    }

    void RenderSystem::Initialize()
    {
        //mRenderer.CreateLight(0, { 0, 10, 0 }, { 1, 0, 0 });
        mManager.SubscribeToEvent<Gep::Event::WindowResize>(this, &Client::RenderSystem::WindowResizeEvent);
        mManager.SubscribeToEvent<Gep::Event::MouseMoved>(this, &Client::RenderSystem::MouseMovedEvent);
        mManager.SubscribeToEvent<Gep::Event::MouseClicked>(this, &Client::RenderSystem::MouseClickedEvent);
    }

    void RenderSystem::Update(float dt)
    {
        mRenderer.Clear();

        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        for (Gep::Entity cameraEntity : cameras)
        {
            const Transform& camTransform = mManager.GetComponent<Transform>(cameraEntity);
            Camera& cam = mManager.GetComponent<Camera>(cameraEntity);

            // convert the camera's rotation to radians
            glm::vec3 camRotation = glm::radians(camTransform.rotation);

            // calculate the camera's right, up, and back vectors from the transforms rotation
            cam.right = { cos(camRotation.y), 0, sin(camRotation.y) };
            cam.up = { sin(camRotation.x) * sin(camRotation.y), cos(camRotation.x), -sin(camRotation.x) * cos(camRotation.y) };
            cam.back = glm::normalize(glm::cross(cam.right, cam.up));

            const glm::mat4 pers = Gep::perspective(cam.viewport, cam.nearPlane, cam.farPlane);
            const glm::mat4 model = glm::mat4({ cam.right, 0 }, { cam.up, 0 }, { cam.back, 0 }, { camTransform.position, 1 });
            const glm::mat4 view = Gep::affine_inverse(model);

            mRenderer.SetCamera(pers, view, { camTransform.position, 1 });

            const std::vector<Gep::Entity>& entities = mManager.GetEntities<Transform, Material>();
            for (Gep::Entity entity : entities)
            {
                const Transform& transform = mManager.GetComponent<Transform>(entity);
                const Material& material = mManager.GetComponent<Material>(entity);

                const glm::mat4 model = Gep::translation_matrix(transform.position)
                                      * Gep::rotation(transform.rotation)
                                      * Gep::scale_matrix(transform.scale);

                mRenderer.SetModel(model);
                mRenderer.SetMaterial(material.diff_coeff, material.spec_coeff, material.spec_exponent);
                
                if (mManager.HasComponent<Texture>(entity))
                {
                    const Texture& texture = mManager.GetComponent<Texture>(entity);
                    mRenderer.SetTexture(texture.textureName);
                }

                const std::vector<Gep::Entity>& lights = mManager.GetEntities<Light, Transform>();
                for (Gep::Entity lightEntity : lights)
                {
                    const Light& light = mManager.GetComponent<Light>(lightEntity);
                    const Transform& lightTransform = mManager.GetComponent<Transform>(lightEntity);

                    mRenderer.AddLight(light.color, lightTransform.position, light.intensity);
                }

                if (material.selected) mRenderer.SetHighlight();

                if (mManager.HasComponent<Client::Light>(entity))
                {
                    Light& light = mManager.GetComponent<Client::Light>(entity);
                    mRenderer.SetSolidColor(light.color * light.intensity);
                }

                mRenderer.DrawMesh(material.meshName);
            }
        }

        HandleInputs(dt);
    }

    void RenderSystem::HandleInputs(float dt)
    {
        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        const float movementSpeed = 10 * dt;


        for (Gep::Entity cam : cameras)
        {
            Transform& transform = mManager.GetComponent<Transform>(cam);
            Camera& camera = mManager.GetComponent<Camera>(cam);

            glm::vec3 forward = glm::normalize(glm::vec3(-camera.back.x, 0.0f, -camera.back.z));
            glm::vec3 rightward = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_W))
            {
                transform.position += forward * movementSpeed;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_S))
            {
                transform.position -= forward * movementSpeed;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_A))
            {
                transform.position -= rightward * movementSpeed;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_D))
            {
                transform.position += rightward * movementSpeed;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE))
            {
                transform.position += glm::vec3(0.0f, movementSpeed, 0.0f);
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_SHIFT))
            {
                transform.position -= glm::vec3(0.0f, movementSpeed, 0.0f);
            }


            GLFWwindow* window = glfwGetCurrentContext();

            static bool isTKeyPressed = false;
            static bool isYKeyPressed = false;

            // Handle T key for textures
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
                if (!isTKeyPressed) { // Key was just pressed
                    mRenderer.ToggleTextures();
                    isTKeyPressed = true;
                }
            }
            else {
                isTKeyPressed = false; // Reset when key is released
            }

            // Handle Y key for wireframes
            if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
                if (!isYKeyPressed) { // Key was just pressed
                    mRenderer.ToggleWireframes();
                    isYKeyPressed = true;
                }
            }
            else {
                isYKeyPressed = false; // Reset when key is released
            }
        }
    }

    void RenderSystem::RenderImGui(float dt)
    {
        ImGui::Begin("Render System");
        ImGui::End();
    }

    void RenderSystem::WindowResizeEvent(const Gep::Event::WindowResize& eventData)
    {
        glViewport(0, 0, eventData.width, eventData.height);

        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        for (Gep::Entity cameraEntity : cameras)
        {
            // update the aspect ratio of the camera
            Camera& cam = mManager.GetComponent<Camera>(cameraEntity);
            cam.viewport.y = cam.viewport.x / eventData.width * eventData.height;
            cam.viewport.z = cam.nearPlane;
            cam.viewport.x = 2.0f * cam.nearPlane * glm::tan(glm::radians(80.0f / 2.0f));
        }
    }

    void RenderSystem::MouseMovedEvent(const Gep::Event::MouseMoved& eventData)
    {
        // only while right mouse button is pressed
        if (glfwGetMouseButton(glfwGetCurrentContext(), GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) return;


        float sensitivity = 0.3f;

        const double dx = (eventData.x - eventData.prevX) * sensitivity;
        const double dy = (eventData.y - eventData.prevY) * sensitivity;

        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();

        for (Gep::Entity cameraEntity : cameras)
        {
            Transform& camTransform = mManager.GetComponent<Transform>(cameraEntity);
            Camera& cam = mManager.GetComponent<Camera>(cameraEntity);
            camTransform.rotation.y += dx;
            camTransform.rotation.x += dy;
            if (camTransform.rotation.x > 89.0f) camTransform.rotation.x = 89.0f;
            if (camTransform.rotation.x < -89.0f) camTransform.rotation.x = -89.0f;
        }
    }

    void RenderSystem::MouseClickedEvent(const Gep::Event::MouseClicked& eventData)
    {
        if (eventData.action == GLFW_PRESS && eventData.button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (eventData.action == GLFW_RELEASE && eventData.button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}


