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

#include "CollisionResource.hpp"

#include <ImGuizmo.h>

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

    void RenderTargetImgui::Draw(EngineManager& em, Entity cameraEntity, const std::function<void()>& drawFunction)
    {
        Client::CollisionResource& collisionResource = em.GetResource<Client::CollisionResource>();
        Client::Camera& camera = em.GetComponent<Client::Camera>(cameraEntity);
        Client::Transform& transform = em.GetComponent<Client::Transform>(cameraEntity);

        const float dt = em.GetDeltaTime();
        float movementSpeed = 5.0f * dt;
        const float sensitivity = 0.1f;



        const glm::vec3 forward = glm::normalize(glm::vec3(-camera.back.x, 0.0f, -camera.back.z));
        const glm::vec3 rightward = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

        if (ImGui::Begin(std::string(GetDisplayName(em, cameraEntity) + "###" + std::to_string(cameraEntity)).c_str()))
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
                Log::Error("Failed to get window handle");
                ImGui::End();
                return;
            }

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

            if (movementEnabled)
            {
                transform.rotation.y += mouseDelta.x * sensitivity;
                transform.rotation.x += mouseDelta.y * sensitivity;

                if (transform.rotation.x > 89.99f) transform.rotation.x = 89.99f;
                if (transform.rotation.x < -89.99f) transform.rotation.x = -89.99f;

                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                {
                    movementSpeed *= 4.0f;
                }

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
                if (glfwGetKey(window, GLFW_KEY_E))
                {
                    transform.position += glm::vec3(0.0f, movementSpeed, 0.0f);
                }
                if (glfwGetKey(window, GLFW_KEY_Q))
                {
                    transform.position -= glm::vec3(0.0f, movementSpeed, 0.0f);
                }
            }

            ImVec2 contentRegionSize = ImGui::GetContentRegionAvail();
            ImVec2 contentRegionPos = ImGui::GetCursorScreenPos();

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
            {
                ImGui::SetWindowFocus();

                ImVec2 mousePos = ImGui::GetMousePos();

                Ray ray = Ray::FromMouse(
                    glm::vec2(mousePos.x - contentRegionPos.x, mousePos.y - contentRegionPos.y),
                    glm::vec2(contentRegionSize.x, contentRegionSize.y),
                    transform.position,
                    camera.GetViewMatrix(transform.position),
                    camera.GetProjectionMatrix()
                );

                std::vector<Gep::Entity> hitEntities = collisionResource.RayCast(em, ray);

                std::string hitOut;
                for (const Gep::Entity entity : hitEntities)
                    hitOut += "[" + std::to_string(entity) + "]";

                if (!hitEntities.empty())
                    Gep::Log::Info("Raycast hit: ", hitOut);
            }


            if (contentRegionSize.x != mSize.x || contentRegionSize.y != mSize.y)
            {
                mSize.x = contentRegionSize.x;
                mSize.y = contentRegionSize.y;
                glViewport(contentRegionPos.x, contentRegionPos.y, contentRegionSize.x, contentRegionSize.y);
            }

            ImVec2 impos = ImGui::GetWindowPos();
            mPosition = *reinterpret_cast<glm::vec2*>(&impos);
            ImGui::Image((void*)(intptr_t)GetTexture(), contentRegionSize, ImVec2(0, 1), ImVec2(1, 0));
            drawFunction();
        }

        ImGui::End();
    }
}
