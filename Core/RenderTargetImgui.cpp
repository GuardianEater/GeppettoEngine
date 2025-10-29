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
        Client::Transform& cameraTransform = em.GetComponent<Client::Transform>(cameraEntity);


        const ImVec2 windowSizeMin(400, 300);
        const ImVec2 windowSizeMax(FLT_MAX, FLT_MAX);
        const float dt = em.GetDeltaTime();
        const float sensitivity = 0.1f;
        float movementSpeed = 25.0f * dt;
        float boostMuliplier = 4.0f;


        const glm::vec3 forward = glm::normalize(glm::vec3(-camera.back.x, 0.0f, -camera.back.z));
        const glm::vec3 rightward = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

        ImGui::SetNextWindowSize(windowSizeMin, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(windowSizeMin, windowSizeMax);
        std::string windowName = GetDisplayName(em, cameraEntity) + "###" + em.GetUUID(cameraEntity).ToString();
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
                Log::Error("Failed to get window handle");
                ImGui::End();
                return;
            }

            UpdateCameraFocus(em, cameraEntity, dt);

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
                cameraTransform.rotation.y += mouseDelta.x * sensitivity;
                cameraTransform.rotation.x += mouseDelta.y * sensitivity;

                // dont go upside down camera
                if (cameraTransform.rotation.x > 89.99f) cameraTransform.rotation.x = 89.99f;
                if (cameraTransform.rotation.x < -89.99f) cameraTransform.rotation.x = -89.99f;

                // speed boost
                if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                    movementSpeed *= boostMuliplier;

                // controls for moveing the camera around
                if (glfwGetKey(window, GLFW_KEY_W))
                    cameraTransform.position += forward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_S))
                    cameraTransform.position -= forward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_A))
                    cameraTransform.position -= rightward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_D))
                    cameraTransform.position += rightward * movementSpeed;
                if (glfwGetKey(window, GLFW_KEY_E))
                    cameraTransform.position += glm::vec3(0.0f, movementSpeed, 0.0f);
                if (glfwGetKey(window, GLFW_KEY_Q))
                    cameraTransform.position -= glm::vec3(0.0f, movementSpeed, 0.0f);
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
            else if (ImGui::IsKeyPressed(ImGuiKey_R) && !ImGui::GetIO().KeyCtrl)
            {
                Gep::Log::Error("Scale is not implemented");

                //currentOperation = ImGuizmo::OPERATION::SCALE;
                //currentMode = ImGuizmo::MODE::LOCAL;
            }

            // prepare gizmos for rendering
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(contentRegionPos.x, contentRegionPos.y, contentRegionSize.x, contentRegionSize.y);
            ImGuizmo::SetOrthographic(false);
            glm::mat4 view = camera.GetViewMatrix(cameraTransform.position);
            glm::mat4 pers = camera.GetProjectionMatrix();

            const auto& selectedEntities = editorResource.GetSelectedEntities();
            std::vector<EntityTransformPair> selectedWithTransform;
            glm::vec3 avgPos(0.0f);

            // get the averages of all selected entities
            for (Entity e : selectedEntities)
            {
                if (em.HasComponent<Client::Transform>(e))
                {
                    auto& tf = em.GetComponent<Client::Transform>(e);
                    selectedWithTransform.emplace_back(e, tf);
                    avgPos += tf.position;
                }
            }

            // if any of the selected entities had a transform get the average model matrix
            if (!selectedWithTransform.empty())
            {
                avgPos /= selectedWithTransform.size();

                glm::mat4 avgModel = glm::translate(glm::mat4(1.0f), avgPos); // this only impacts the placement of the gizmo

                constexpr float snap[3] = { 0.1f, 0.1f, 0.1f };
                glm::mat4 deltaMatrix(1.0f);
                
                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(pers), currentOperation, currentMode, glm::value_ptr(avgModel), glm::value_ptr(deltaMatrix), snap))
                {
                    glm::vec3 pos{ 0 }, rot{ 0 }, scl{ 0 };
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(deltaMatrix), glm::value_ptr(pos), glm::value_ptr(rot), glm::value_ptr(scl));

                    for (EntityTransformPair etp : selectedWithTransform)
                    {
                        auto& tf = etp.transform;

                        glm::mat4 model = Gep::translation_matrix(tf.position)
                                        * Gep::rotation(tf.rotation)
                                        * Gep::scale_matrix(tf.scale); // scale aafsgd
                            
                        glm::mat4 newModel = deltaMatrix * model;

                        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(newModel), glm::value_ptr(tf.position), glm::value_ptr(tf.rotation), glm::value_ptr(tf.scale));
                    }
                }
                guizmoActive = true;
            }
            else
                guizmoActive = false;

            // when the window Is clicked set the focus and fire a ray
            if (ImGui::IsKeyPressed(ImGuiKey_F) && !selectedWithTransform.empty())
            {
                StartCameraFocus(em, cameraEntity, avgPos, ComputeContainingScale(selectedWithTransform, avgPos));
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hovered)
            {
                ImGui::SetWindowFocus();

                ImVec2 mousePos = ImGui::GetMousePos();

                // create a ray based on the content region
                Ray ray = Ray::FromMouse(
                    glm::vec2(mousePos.x - contentRegionPos.x, mousePos.y - contentRegionPos.y),
                    glm::vec2(contentRegionSize.x, contentRegionSize.y),
                    cameraTransform.position,
                    camera.GetViewMatrix(cameraTransform.position),
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

    void RenderTargetImgui::StartCameraFocus(EngineManager& em, Gep::Entity camera, const glm::vec3& targetPosition, float targetScale)
    {
        Client::Transform& cameraTransform = em.GetComponent<Client::Transform>(camera);

        cameraTransform.rotation = glm::mod(cameraTransform.rotation, glm::vec3(360.0f)); // prevents camera from unwinding
        const glm::vec3 camPosition = cameraTransform.position;

        glm::vec3 targetDirection;
        if (targetPosition == camPosition)
            targetDirection = { 0.333f, 0.333f, 0.333f };
        else
            targetDirection = glm::normalize(targetPosition - camPosition);

        const float distance = glm::distance(targetPosition, camPosition);

        // atan2 handles full 360-degree rotations
        float pitch = glm::degrees(asin(glm::clamp(-targetDirection.y, -1.0f, 1.0f)));
        float yaw = glm::degrees(atan2(targetDirection.x, -targetDirection.z));
        float roll = 0.0f;

        mCameraTargetPosition = camPosition + (targetDirection * (distance - (targetScale * 1.2f)));
        mCameraTargetPosition -= targetDirection * 1.0f;
        mCameraTargetRotation = glm::mod(glm::vec3{ pitch, yaw, roll }, 360.0f);
        mCameraLerping = true;
    }

    void RenderTargetImgui::UpdateCameraFocus(EngineManager& em, Gep::Entity camera, float dt)
    {
        if (!mCameraLerping) return;

        glm::vec3& currentRotation = em.GetComponent<Client::Transform>(camera).rotation;
        glm::vec3& currentPosition = em.GetComponent<Client::Transform>(camera).position;

        currentRotation = glm::mix(currentRotation, mCameraTargetRotation, 0.01f);
        currentPosition = glm::mix(currentPosition, mCameraTargetPosition, 0.01f);

        if (glm::length(mCameraTargetRotation - currentRotation) < 0.01 &&
            glm::length(mCameraTargetPosition - currentPosition) < 0.01)
        {
            mCameraLerping = false;
        }
    }

    float RenderTargetImgui::ComputeContainingScale(const std::vector<EntityTransformPair>& etps, const glm::vec3& avgPos)
    {
        float maxDistance = 0.0f;

        for (const EntityTransformPair& etp : etps)
        {
            float distanceToCenter = glm::length(etp.transform.position - avgPos);
            float totalDistance = distanceToCenter + std::max({ etp.transform.scale.x,etp.transform.scale.y,etp.transform.scale.z });
            maxDistance = std::max(maxDistance, totalDistance);
        }

        return maxDistance;
    }
}
