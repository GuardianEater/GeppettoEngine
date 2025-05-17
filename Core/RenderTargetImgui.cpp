/*****************************************************************//**
 * \file   RenderTargetImgui.cpp
 * \brief  renders to an imgui window
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   February 2025
 *********************************************************************/

#include "pch.hpp"
#include "RenderTargetImgui.hpp"

#include "Transform.hpp"
#include "CameraComponent.hpp"
#include "EngineManager.hpp"

#include "CollisionResource.hpp"
#include "EditorResource.hpp"

#include <ImGuizmo.h>

namespace Gep
{
    static std::string GetDisplayName(EngineManager& em, Entity entity)
    {
        std::string name = em.GetName(entity);
        if (!name.empty())
        {
            return name;
        }

        return "Entity: " + std::to_string(entity);
    }

    void RenderTargetImgui::Draw(EngineManager& em, Entity cameraEntity, const std::function<void()>& drawFunction)
    {
        Client::CollisionResource& collisionResource = em.GetResource<Client::CollisionResource>();
        Client::EditorResource& editorResource = em.GetResource<Client::EditorResource>();

        Client::Camera& camera = em.GetComponent<Client::Camera>(cameraEntity);
        Client::Transform& transform = em.GetComponent<Client::Transform>(cameraEntity);

        const ImVec2 windowSizeMin(400, 300);
        const ImVec2 windowSizeMax(FLT_MAX, FLT_MAX);
        const float dt = em.GetDeltaTime();
        const float sensitivity = 0.1f;
        float movementSpeed = 5.0f * dt;

        const glm::vec3 forward = glm::normalize(glm::vec3(-camera.back.x, 0.0f, -camera.back.z));
        const glm::vec3 rightward = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

        ImGui::SetNextWindowSize(windowSizeMin, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(windowSizeMin, windowSizeMax);
        if (ImGui::Begin((GetDisplayName(em, cameraEntity) + "###" + em.GetUUID(cameraEntity).ToString()).c_str()))
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
                // rotating the camera around
                transform.rotation.y += mouseDelta.x * sensitivity;
                transform.rotation.x += mouseDelta.y * sensitivity;

                // dont go upside down camera
                if (transform.rotation.x > 89.99f) transform.rotation.x = 89.99f;
                if (transform.rotation.x < -89.99f) transform.rotation.x = -89.99f;

                // speed boost
                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                    movementSpeed *= 4.0f;

                // controls for moveing the camera around
                if (glfwGetKey(window, GLFW_KEY_W))
                    transform.position += forward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_S))
                    transform.position -= forward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_A))
                    transform.position -= rightward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_D))
                    transform.position += rightward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_E))
                    transform.position += glm::vec3(0.0f, movementSpeed, 0.0f);
                if (glfwGetKey(window, GLFW_KEY_Q))
                    transform.position -= glm::vec3(0.0f, movementSpeed, 0.0f);
            }

            ImVec2 contentRegionSize = ImGui::GetContentRegionAvail();
            ImVec2 contentRegionPos = ImGui::GetCursorScreenPos();

            // update the content region size/position if either were changed
            if (contentRegionSize.x != mSize.x || contentRegionSize.y != mSize.y ||
                contentRegionPos.x != mPosition.x || contentRegionPos.y != mPosition.y)
            {
                mSize = *reinterpret_cast<glm::vec2*>(&contentRegionSize);
                mPosition = *reinterpret_cast<glm::vec2*>(&contentRegionPos);

                glViewport(contentRegionPos.x, contentRegionPos.y, contentRegionSize.x, contentRegionSize.y);
            }

            // draw to everything to the imgui texture
            ImGui::Image((void*)(intptr_t)GetTexture(), contentRegionSize, ImVec2(0, 1), ImVec2(1, 0)); // flipped uvs
            drawFunction(); // user function might be useful

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
            if (!movementEnabled)
            {
                if (ImGui::IsKeyDown(ImGuiKey_W))
                {
                    currentOperation = ImGuizmo::OPERATION::TRANSLATE;
                    currentMode = ImGuizmo::MODE::WORLD;
                }
                else if (ImGui::IsKeyDown(ImGuiKey_E))
                {
                    currentOperation = ImGuizmo::OPERATION::ROTATE;
                    currentMode = ImGuizmo::MODE::LOCAL;
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_R))
                {
                    Gep::Log::Error("Scale is not implemented");

                    //currentOperation = ImGuizmo::OPERATION::SCALE;
                    //currentMode = ImGuizmo::MODE::LOCAL;
                }
            }

            // prepare gizmos for rendering
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(contentRegionPos.x, contentRegionPos.y, contentRegionSize.x, contentRegionSize.y);
            glm::mat4 view = camera.GetViewMatrix(transform.position);
            glm::mat4 pers = camera.GetProjectionMatrix();

            const auto& selectedEntities = editorResource.GetSelectedEntities();
            glm::vec3 avgPos(0.0f);
            int count = 0;

            // if there is an entity selected use its center as an average
            if (selectedEntities.size() == 1)
            {
                Entity e = *selectedEntities.begin();
                if (em.HasComponent<Client::Transform>(e))
                {
                    const auto& tf = em.GetComponent<Client::Transform>(e);
                    avgPos = tf.position;

                    ++count;
                }

            }

            // get the averages of all selected entities
            for (Entity e : selectedEntities)
            {
                if (em.HasComponent<Client::Transform>(e))
                {
                    const auto& tf = em.GetComponent<Client::Transform>(e);
                    avgPos += tf.position;

                    ++count;
                }
            }

            // if any of the selected entities had a transform get the average model matrix
            if (count != 0)
            {
                avgPos /= count;

                glm::mat4 avgModel = glm::translate(glm::mat4(1.0f), avgPos); // this only impacts the placement of the gizmo

                constexpr float snap[3] = { 0.1f, 0.1f, 0.1f };
                glm::mat4 deltaMatrix(1.0f);
                
                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(pers), currentOperation, currentMode, glm::value_ptr(avgModel), glm::value_ptr(deltaMatrix), snap))
                {
                    glm::vec3 pos{ 0 }, rot{ 0 }, scl{ 0 };
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(deltaMatrix), glm::value_ptr(pos), glm::value_ptr(rot), glm::value_ptr(scl));

                    for (Entity e : selectedEntities)
                    {
                        if (em.HasComponent<Client::Transform>(e))
                        {
                            auto& tf = em.GetComponent<Client::Transform>(e);

                            glm::mat4 model = Gep::translation_matrix(tf.position)
                                            * Gep::rotation(tf.rotation)
                                            * Gep::scale_matrix(tf.scale); // scale aafsgd
                            
                            glm::mat4 newModel = deltaMatrix * model;

                            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(newModel), glm::value_ptr(tf.position), glm::value_ptr(tf.rotation), glm::value_ptr(tf.scale));
                        }
                    }
                }
                guizmoActive = true;
            }
            else
                guizmoActive = false;

            // when the window Is clicked set the focus and fire a ray
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
            {
                ImGui::SetWindowFocus();

                ImVec2 mousePos = ImGui::GetMousePos();

                // create a ray based on the content region
                Ray ray = Ray::FromMouse(
                    glm::vec2(mousePos.x - contentRegionPos.x, mousePos.y - contentRegionPos.y),
                    glm::vec2(contentRegionSize.x, contentRegionSize.y),
                    transform.position,
                    camera.GetViewMatrix(transform.position),
                    camera.GetProjectionMatrix()
                );

                std::vector<Gep::Entity> hitEntities = collisionResource.RayCast(em, ray);

                // if the gizmo is not in use or if the gizmo is not enabled (for some reason IsOver returns true when its not active)
                if (!ImGuizmo::IsOver() || !guizmoActive)
                {
                    if (!hitEntities.empty())
                        editorResource.SmartSelectEntity(hitEntities.front(), window);
                    else if (!glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                        editorResource.DeselectAll();
                }
            }
        }

        ImGui::End();
    }

    void RenderTargetImgui::HandleGuizmo(EngineManager& em)
    {
    }
}
