/*****************************************************************//**
 * \file   ImGuiSystem.cpp
 * \brief  implementation of the ImGuiSystem
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "ImGuiSystem.hpp"

#include "CameraComponent.hpp"
#include "Transform.hpp"
namespace Client
{
    ImGuiSystem::ImGuiSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    std::string ImGuiSystem::GetEntityDisplayName(Gep::Entity entity)
    {
        std::string displayName = "Entity: " + std::to_string(entity);
        if (mManager.HasComponent<Identification>(entity))
        {
            Identification& id = mManager.GetComponent<Identification>(entity);

            if (!id.name.empty())
                displayName = id.name;
        }

        displayName += "###" + std::to_string(entity);

        return displayName;
    }

    void ImGuiSystem::DrawInspectorPanel()
    {
        if (mSelectedEntities.size() == 0) return;

        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
        if (mSelectedEntities.size() > 1)
        {
            for (Gep::Entity entity : mSelectedEntities)
            {
                std::string displayName = GetEntityDisplayName(entity);
                ImGui::Text(displayName.c_str());
            }
            ImGui::End(); // Inspector
            return;
        }

        Gep::Entity entity = *mSelectedEntities.begin();

        std::string displayName = GetEntityDisplayName(entity);

        ImGui::Text(displayName.c_str());
        ImGui::Dummy({ 0.0f, 10.0f });

        mManager.ForEachComponent(entity, [&](const Gep::ComponentData& componentData)
        {
            mComponentInspectorPanels[componentData.index](entity);
        });

        ImGui::Dummy({ 0.0f, 10.0f });

        ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        ImGui::PushStyleColor(ImGuiCol_Header, buttonColor);

        // Calculate the text size and available space
        if (ImGui::CollapsingHeader("Add Component"))
        {
            for (auto& componentData : mManager.GetComponentDatas())
            {
                if (componentData.has(entity)) continue;

                if (ImGui::Button(componentData.name.c_str(), { ImGui::GetContentRegionAvail().x, 30.0f }))
                {
                    componentData.add(entity);
                }
            }
        }

        if (ImGui::Button("Duplicate", { ImGui::GetContentRegionAvail().x, 30.0f }))
        {
            mManager.DuplicateEntity(entity);
        }

        ImGui::PopStyleColor(); // button color
        ImGui::End(); // Inspector
    }

    void ImGuiSystem::Update(float dt)
    {
        std::vector<Gep::Entity>& entities = mManager.GetEntities();

        // get only the entities that are parents
        std::vector<Gep::Entity> parents;
        for (Gep::Entity entity : entities)
        {
            if (!mManager.HasParent(entity))
            {
                parents.push_back(entity);
            }
        }

        ImGui::Begin("Entities");

        if (ImGui::Button("Create", { ImGui::GetContentRegionAvail().x , 30.0f}))
        {
            mManager.CreateEntity();
        }

        DrawEntities(parents);

        // detach entities if they are dropped on the hierarchy panel
        ImGui::Dummy(ImGui::GetContentRegionAvail());
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
            {
                Gep::Entity droppedEntity = *(const Gep::Entity*)payload->Data;
                mManager.DetachEntity(droppedEntity);
            }
            ImGui::EndDragDropTarget();
        }

        // clears selected entities if the background is clicked
        if (ImGui::IsItemClicked())
        {
            mSelectedEntities.clear();
        }

        // delete selected entities
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        {
            for (Gep::Entity selectedEntity : mSelectedEntities)
            {
                mManager.MarkEntityForDestruction(selectedEntity);
            }
        }

        // duplicate selected entities
        if (ImGui::IsKeyPressed(ImGuiKey_D, false) && ImGui::GetIO().KeyCtrl)
        {
            for (Gep::Entity selectedEntity : mSelectedEntities)
            {
                mManager.DuplicateEntity(selectedEntity);
            }
        }

        DrawInspectorPanel();

        ImGui::End(); // Entities
    }

    void ImGuiSystem::DrawEntities(const std::vector<Gep::Entity>& entities)
    {
        size_t index = 0;
        for (Gep::Entity entity : entities)
        {
            ImGui::PushID(entity);

            std::string displayName = GetEntityDisplayName(entity);

            ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // Increase padding
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f)); // Increase vertical padding
            
            // change the color of tree node selected
            ImVec4 selectedColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            ImVec4 defaultColor  = ImGui::GetStyleColorVec4(ImGuiCol_Header);
            ImVec4 color         = mSelectedEntities.contains(entity) ? selectedColor : defaultColor;
            ImVec4 hoverColor    = ImVec4(color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, 1.0f);

            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hoverColor);
            ImGui::PushStyleColor(ImGuiCol_Header, mSelectedEntities.contains(entity) ? selectedColor : defaultColor);

            bool isOpen = ImGui::TreeNodeEx(displayName.c_str()
                , ImGuiTreeNodeFlags_OpenOnArrow 
                | ImGuiTreeNodeFlags_SpanAvailWidth 
                | (mSelectedEntities.contains(entity) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);

            static bool lerpingCamera = false;
            static bool movingCamera = false;
            static glm::vec3 targetRotation{};
            if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
            {
                // should probably change this so it only does it on editor camera
                const std::vector<Gep::Entity>& activeCameras = mManager.GetEntities<Client::Camera, Client::Transform>();
                for (Gep::Entity camera : activeCameras)
                {
                    if (mManager.HasComponent<Client::Transform>(entity)) 
                    {
                        const glm::vec3 camPosition = mManager.GetComponent<Client::Transform>(camera).position;
                        const glm::vec3 targetPosition = mManager.GetComponent<Client::Transform>(entity).position;
                        const glm::vec3 targetDirection = glm::normalize(targetPosition - camPosition);
                        const float distance = glm::distance(targetPosition, camPosition);

                        // atan2 handles full 360-degree rotations
                        float pitch = glm::degrees(asin(glm::clamp(-targetDirection.y, -1.0f, 1.0f)));
                        float yaw = glm::degrees(atan2(targetDirection.x, -targetDirection.z));
                        float roll = 0.0f;

                        targetRotation = { pitch, yaw, roll };
                        lerpingCamera = true;
                    }
                }
            }

            if (lerpingCamera)
            {
                const std::vector<Gep::Entity>& activeCameras = mManager.GetEntities<Client::Camera, Client::Transform>();
                for (Gep::Entity camera : activeCameras)
                {
                    glm::vec3& currentRotation = mManager.GetComponent<Client::Transform>(camera).rotation;

                    currentRotation = glm::mix(currentRotation, targetRotation, 0.01);

                    if (glm::length(targetRotation - currentRotation) < 0.001)
                    {
                        lerpingCamera = false;
                    }
                }
            }

            // bool open = ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding);
            if (ImGui::IsItemClicked()) 
            {
                // Multi-selection logic (Ctrl or Shift key)
                const bool isCtrlPressed = ImGui::GetIO().KeyCtrl;
                const bool isShiftPressed = ImGui::GetIO().KeyShift;

                // If the control key is pressed, add the entity to the selection, or remove it if it is already selected
                if (isCtrlPressed)
                {
                    if (mSelectedEntities.contains(entity))
                        mSelectedEntities.erase(entity);
                    else
                        mSelectedEntities.insert(entity);
                }
                else
                {
                    mSelectedEntities.clear();
                    mSelectedEntities.insert(entity);
                }

                static size_t lastSelectedIndex = std::numeric_limits<size_t>::max(); // Invalid index initially

                // If the shift key is pressed, select all entities between the last selected entity and the current entity
                if (isShiftPressed)
                {
                    if (lastSelectedIndex < entities.size())
                    {
                        size_t currentIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
                        if (lastSelectedIndex < currentIndex)
                        {
                            for (size_t i = lastSelectedIndex; i <= currentIndex; ++i)
                            {
                                mSelectedEntities.insert(entities[i]);
                            }
                        }
                        else
                        {
                            for (size_t i = currentIndex; i <= lastSelectedIndex; ++i)
                            {
                                mSelectedEntities.insert(entities[i]);
                            }
                        }
                    }
                }
                else
                {
                    // Update the last selected index
                    lastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
                }
            }

            // Add context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    mManager.MarkEntityForDestruction(entity);
                }
                if (ImGui::MenuItem("Duplicate"))
                {
                    mManager.DuplicateEntity(entity);
                }
                ImGui::EndPopup();
            }

            // Add drag and drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(Gep::Entity));
                ImGui::Text("Dragging %s", displayName.c_str());
                ImGui::EndDragDropSource();
            }

            // Add drag and drop target
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
                {
                    Gep::Entity droppedEntity = *(const Gep::Entity*)payload->Data;
                    mManager.AttachEntity(entity, droppedEntity);
                }
                ImGui::EndDragDropTarget();
            }

            if (isOpen)
            {
                DrawEntities(mManager.GetChildren(entity));
                ImGui::TreePop();
            }

            ImGui::PopID();
            ++index;
        }
    }

    template <>
    void ImGuiSystem::DrawType<float>(const std::string_view name, float& t)
    {
        ImGui::DragFloat(name.data(), &t);
    }

    template <>
    void ImGuiSystem::DrawType<int>(const std::string_view name, int& t)
    {
        ImGui::InputInt(name.data(), &t);
    }

    template <>
    void ImGuiSystem::DrawType<std::string>(const std::string_view name, std::string& t)
    {
        ImGui::InputText(name.data(), &t);
    }

    template <>
    void ImGuiSystem::DrawType<glm::vec3>(const std::string_view name, glm::vec3& t)
    {
        ImGui::DragFloat3(name.data(), &t.x);
    }

    template <>
    void ImGuiSystem::DrawType<glm::vec4>(const std::string_view name, glm::vec4& t)
    {
        ImGui::DragFloat4(name.data(), &t.x);
    }

    template <>
    void ImGuiSystem::DrawType<size_t>(const std::string_view name, size_t& t)
    {
        ImGui::InputScalar(name.data(), ImGuiDataType_U64, &t);
    }

    template <>
    void ImGuiSystem::DrawType<bool>(const std::string_view name, bool& t)
    {
        ImGui::Checkbox(name.data(), &t);
    }

    template <>
    void ImGuiSystem::DrawType<char>(const std::string_view name, char& t)
    {
        ImGui::InputScalar(name.data(), ImGuiDataType_S8, &t);
    }
}
