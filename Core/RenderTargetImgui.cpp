/*****************************************************************//**
 * \file   RenderTargetImgui.cpp
 * \brief  renders to an imgui window
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "RenderTargetImgui.hpp"

#include "Transform.hpp"
#include "CameraComponent.hpp"
#include "EngineManager.hpp"
#include "Identification.hpp"

namespace Gep
{
    static std::string GetDisplayName(EngineManager& em, Entity entity)
    {
        if (em.HasComponent<Client::Identification>(entity))
        {
            Client::Identification& id = em.GetComponent<Client::Identification>(entity);
            return id.name;
        }
        return "Entity: " + std::to_string(entity);
    }

    void RenderTargetImgui::Draw(EngineManager& em, Entity cameraEntity)
    {
        Client::Camera& camera = em.GetComponent<Client::Camera>(cameraEntity);
        Client::Transform& transform = em.GetComponent<Client::Transform>(cameraEntity);

        const float movementSpeed = 0.1f;
        const float sensitivity = 0.1f;

        const glm::vec3 forward = glm::normalize(glm::vec3(-camera.back.x, 0.0f, -camera.back.z));
        const glm::vec3 rightward = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

        if (ImGui::Begin(std::string(GetDisplayName(em, cameraEntity) + "###" + std::to_string(cameraEntity)).c_str()))
        {
            // get mouse delta and wether or not right click is pressed to rotate the camera
            bool rightClick = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
            bool focused = ImGui::IsWindowFocused();

            // gets the underlying window
            ImGuiViewport* viewport = ImGui::GetWindowViewport();
            GLFWwindow* window = nullptr;
            if (viewport && viewport->PlatformHandle)
                window = (GLFWwindow*)viewport->PlatformHandle;
            else
            {
                Log::Error("Failed to get window handle");
                ImGui::End();
                return;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            if (rightClick && focused)
            {
                transform.rotation.y += mouseDelta.x * sensitivity;
                transform.rotation.x += mouseDelta.y * sensitivity;
                if (transform.rotation.x > 89.0f) transform.rotation.x = 89.0f;
                if (transform.rotation.x < -89.0f) transform.rotation.x = -89.0f;

                if (glfwGetKey(window, GLFW_KEY_W))
                {
                    transform.position += forward * movementSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_S))
                {
                    transform.position -= forward * movementSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_A))
                {
                    transform.position -= rightward * movementSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_D))
                {
                    transform.position += rightward * movementSpeed;
                }
                if (glfwGetKey(window, GLFW_KEY_SPACE))
                {
                    transform.position += glm::vec3(0.0f, movementSpeed, 0.0f);
                }
                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                {
                    transform.position -= glm::vec3(0.0f, movementSpeed, 0.0f);
                }
            }

            ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x != mSize.x || size.y != mSize.y)
            {
                mSize.x = size.x;
                mSize.y = size.y;
                glViewport(0, 0, size.x, size.y);
            }

            ImGui::Image((void*)(intptr_t)GetTexture(), size, ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();
    }
}
