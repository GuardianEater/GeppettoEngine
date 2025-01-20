/*****************************************************************//**
 * \file   RenderSystem.cpp
 * \brief  System that renders objects
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "RenderSystem.hpp"

namespace Client
{
    RenderSystem::RenderSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mRenderer()
    {
        mRenderer.LoadVertexShader("assets\\shaders\\PhongRender.vert");
        mRenderer.LoadFragmentShader("assets\\shaders\\PhongRender.frag");
        mRenderer.Compile();

        mRenderer.BackfaceCull();
        mRenderer.SetAmbientLight({ 0.5, 0.5, 0.5 });

        mRenderer.LoadImage("test", "assets\\textures\\test.jpg");
        mRenderer.LoadMesh("Sphere", Gep::SphereMesh(10, 10));
        mRenderer.LoadMesh("Cube", Gep::CubeMesh());
    }

    RenderSystem::~RenderSystem()
    {
        mRenderer.UnloadMesh("Sphere");
        mRenderer.UnloadMesh("Cube");
    }

    void RenderSystem::Initialize()
    {
        //mRenderer.CreateLight(0, { 0, 10, 0 }, { 1, 0, 0 });
    }

    void RenderSystem::Update(float dt)
    {
        mRenderer.Clear();

        const std::vector<Gep::Entity>& cameras = mManager.GetEntities<Transform, Camera>();
        for (Gep::Entity cameraEntity : cameras)
        {
            const Transform& camTransform = mManager.GetComponent<Transform>(cameraEntity);
            const Camera& cam = mManager.GetComponent<Camera>(cameraEntity);

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
                mRenderer.SetTexture(material.textureName);
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

            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_W))
            {
                const glm::vec3 forward = -glm::normalize(camera.back) * movementSpeed;
                transform.position += forward;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_S))
            {
                const glm::vec3 backward = glm::normalize(camera.back) * movementSpeed;
                transform.position += backward;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_A))
            {
                const glm::vec3 leftward = -glm::normalize(camera.right) * movementSpeed;
                transform.position += leftward;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_D))
            {
                const glm::vec3 rightward = glm::normalize(camera.right) * movementSpeed;
                transform.position += rightward;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE))
            {
                const glm::vec3 upward = glm::normalize(camera.up) * movementSpeed;
                transform.position += upward;
            }
            if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_SHIFT))
            {
                const glm::vec3 downward = -glm::normalize(camera.up) * movementSpeed;
                transform.position += downward;
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

    void RenderSystem::KeyEvent(const Gep::Event::KeyPressed& eventData)
    {
    }
}


