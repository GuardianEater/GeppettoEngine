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
#include "Material.hpp"

#include "imgui_te_engine.h"

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

        displayName += " [" + std::to_string(entity) + "]";

        return displayName;
    }

    void ImGuiSystem::DrawInspectorPanel()
    {
        if (mSelectedEntities.size() == 0)
        {
            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::Text("No Entity Selected");
            ImGui::End(); // Inspector
            return;
        }

        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
        if (mSelectedEntities.size() > 1)
        {
            ImGui::Text("%d Entities Selected", mSelectedEntities.size());

            ImGui::Separator();

            for (Gep::Entity entity : mSelectedEntities)
            {
                std::string displayName = GetEntityDisplayName(entity);
                ImGui::Text(displayName.c_str());
            }
            ImGui::End(); // Inspector
            return;
        }

        Gep::Entity entity = *mSelectedEntities.begin();
        if (mManager.HasComponent<Client::Material>(entity))
        {
            Client::Material& material = mManager.GetComponent<Client::Material>(entity);
            material.selected = true;
        }

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

        ImGui::PopStyleColor(); // button color
        ImGui::End(); // Inspector
    }

    void ImGuiSystem::DrawInfoPanel()
    {
        ImGui::SetNextWindowSize({ 230.0f, 300.0f });
        ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize);

        ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("Latency: %.2f ms", ImGui::GetIO().DeltaTime * 1000.0f);
        ImGui::Separator();

        ImGui::Text("Entities: %d", mManager.GetEntities().size());
        const auto& components = mManager.GetComponentDatas();
        ImGui::Text("Components Registered: %d", components.size());
        ImGui::Separator();
        

        if (ImGui::BeginTable("ComponentTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            for (const auto& component : components)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", component.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%u", component.array->size());
                ImGui::TableNextColumn();
                ImGui::Text("%u", component.index);
            }

            ImGui::EndTable();
        }
        
        ImGui::End(); // Info
    }

    void ImGuiSystem::DrawExtras()
    {
        ImGui::Begin("Extra");
        static bool showDemoWindow = false;
        static bool showStyleWindow = false;
        static bool showMetricsWindow = false;

        ImGui::Checkbox("Demo Window", &showDemoWindow);
        ImGui::Checkbox("Style Window", &showStyleWindow);
        ImGui::Checkbox("Metrics Window", &showMetricsWindow);

        if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
        if (showStyleWindow) { ImGui::Begin("Style");  ImGui::ShowStyleEditor(); ImGui::End(); }
        if (showMetricsWindow) ImGui::ShowMetricsWindow();
        ImGui::End(); // Extra
    }

    bool ImGuiSystem::DrawEntityNode(Gep::Entity entity)
    {
        // the identification component aquired here returns the wrong name
        std::string displayName = GetEntityDisplayName(entity);

        ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // Increase padding
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f)); // Increase vertical padding

        // change the color of tree node selected
        ImVec4 selectedColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        ImVec4 defaultColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
        ImVec4 color = mSelectedEntities.contains(entity) ? selectedColor : defaultColor;
        ImVec4 hoverColor = ImVec4(color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_Header, mSelectedEntities.contains(entity) ? selectedColor : defaultColor);

        bool isOpen = ImGui::TreeNodeEx(displayName.c_str()
            , ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | (mSelectedEntities.contains(entity) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        return isOpen;
    }

    void ImGuiSystem::StartCameraFocus(Gep::Entity entity)
    {
        // should probably change this so it only does it on editor camera
        const std::vector<Gep::Entity>& activeCameras = mManager.GetEntities<Client::Camera, Client::Transform>();
        for (Gep::Entity camera : activeCameras)
        {
            if (mManager.HasComponent<Client::Transform>(entity) && !mManager.HasComponent<Client::Camera>(entity))
            {
                const glm::vec3 camPosition = mManager.GetComponent<Client::Transform>(camera).position;
                const glm::vec3 targetPosition = mManager.GetComponent<Client::Transform>(entity).position;
                const glm::vec3 targetDirection = glm::normalize(targetPosition - camPosition);
                const float distance = glm::distance(targetPosition, camPosition);

                // atan2 handles full 360-degree rotations
                float pitch = glm::degrees(asin(glm::clamp(-targetDirection.y, -1.0f, 1.0f)));
                float yaw = glm::degrees(atan2(targetDirection.x, -targetDirection.z));
                float roll = 0.0f;

                mCameraTargetRotation = { pitch, yaw, roll };
                mCameraLerping = true;
            }
        }
    }

    void ImGuiSystem::UpdateCameraFocus(float dt)
    {
        if (mCameraLerping)
        {
            const std::vector<Gep::Entity>& activeCameras = mManager.GetEntities<Client::Camera, Client::Transform>();
            for (Gep::Entity camera : activeCameras)
            {
                glm::vec3& currentRotation = mManager.GetComponent<Client::Transform>(camera).rotation;

                currentRotation = glm::mix(currentRotation, mCameraTargetRotation, 0.1f);

                if (glm::length(mCameraTargetRotation - currentRotation) < 0.001)
                {
                    mCameraLerping = false;
                }
            }
        }
    }

    std::vector<Gep::Entity> ImGuiSystem::SearchEntities(const std::vector<Gep::Entity>& entities, const std::string& searchTerm)
    {
        std::vector<Gep::Entity> result;

        for (Gep::Entity entity : entities) 
        {
            std::string displayName = GetEntityDisplayName(entity);
            if (displayName.find(searchTerm) != std::string::npos)
            {
                result.push_back(mManager.GetRoot(entity));
            }
        }

        // remove duplicates
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());

        return result;
    }

    template <typename FunctionType>
    requires std::invocable<FunctionType, Gep::Entity>
    void ImGuiSystem::EntitiesDragDropTarget(FunctionType func)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
            {
                Gep::Entity* droppedEntities = (Gep::Entity*)payload->Data;
                size_t droppedEntityCount = payload->DataSize / sizeof(Gep::Entity);
                std::set<Gep::Entity> droppedEntitiesSet(droppedEntities, droppedEntities + droppedEntityCount);

                for (Gep::Entity droppedEntity : droppedEntitiesSet)
                {
                    func(droppedEntity);
                }
                mSelectedEntities.clear();

            }
            ImGui::EndDragDropTarget();
        }
    }

    void ImGuiSystem::Update(float dt)
    {
        DrawInfoPanel();
        DrawExtras();
        
        ImGui::Begin("Entities");
        if (ImGui::Button("Create", { ImGui::GetContentRegionAvail().x , 30.0f}))
        {
            mManager.CreateEntity();
        }

        // search bar
        static std::string search = "";
        
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("###Search", "Search", &search);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        mEntities = mManager.GetEntities();
        // set sorting here
        // can organize by search filter aswell

        // get only the entities that are parents
        std::vector<Gep::Entity> parents;
        if (search.empty())
        {
            for (Gep::Entity entity : mEntities)
            {
                if (!mManager.HasParent(entity))
                {
                    parents.push_back(entity);
                }
            }
        }
        else
        {
            parents = SearchEntities(mEntities, search);
        }


        DrawEntities(parents, dt);

        // detach entities if they are dropped into any open space
        ImGui::Dummy(ImGui::GetContentRegionAvail());

        EntitiesDragDropTarget([&](Gep::Entity entity)
        {
            mManager.DetachEntity(entity);
        });

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
            mSelectedEntities.clear();
        }

        // duplicate selected entities
        if (ImGui::IsKeyPressed(ImGuiKey_D, false) && ImGui::GetIO().KeyCtrl)
        {
            for (Gep::Entity selectedEntity : mSelectedEntities)
            {
                mManager.DuplicateEntity(selectedEntity);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
            if (mSelectedEntities.size() == 1)
            {
                StartCameraFocus(*mSelectedEntities.begin());
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_A, false) && ImGui::GetIO().KeyCtrl)
        {
            mSelectedEntities.clear();
            for (Gep::Entity entity : mEntities)
            {
                mSelectedEntities.insert(entity);
            }
        }

        DrawInspectorPanel();
        UpdateCameraFocus(dt);

        ImGui::End(); // Entities
    }

    void ImGuiSystem::DrawEntities(const std::vector<Gep::Entity>& entities, float dt)
    {
        for (Gep::Entity entity : entities)
        {
            ImGui::PushID(entity);

            const std::string displayName = GetEntityDisplayName(entity);
            bool isOpen = DrawEntityNode(entity);

            // Multi-selection logic (Ctrl or Shift key)
            const bool isCtrlPressed = ImGui::GetIO().KeyCtrl;
            const bool isShiftPressed = ImGui::GetIO().KeyShift;
            static size_t lastSelectedIndex = std::numeric_limits<size_t>::max(); // Invalid index initially
            
            // default selection
            if (ImGui::IsItemClicked() && !isCtrlPressed && !isShiftPressed && mSelectedEntities.size() <= 1)
            {
                mSelectedEntities.clear();
                mSelectedEntities.insert(entity);
                lastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
            }

            // same as default but if there are multiple entities selected, doesn't deselect until released
            else if (ImGui::IsItemDeactivated() && !isCtrlPressed && !isShiftPressed && mSelectedEntities.size() > 1)
            {
                mSelectedEntities.clear();
                mSelectedEntities.insert(entity);
                lastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
            }

            // will select the entity if it is clicked, and unselect if it is clicked again
            else if (ImGui::IsItemClicked() && isCtrlPressed && !isShiftPressed)
            {
                if (mSelectedEntities.contains(entity))
                    mSelectedEntities.erase(entity);
                else
                {
                    mSelectedEntities.insert(entity);
                    lastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
                }
            }

            // multiselect with shift key
            else if (ImGui::IsItemClicked() && !isCtrlPressed && isShiftPressed)
            {
                mSelectedEntities.clear();
                if (lastSelectedIndex < entities.size())
                {
                    size_t currentIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
                    if (lastSelectedIndex < currentIndex)
                    {
                        for (size_t i = lastSelectedIndex; i <= currentIndex; ++i)
                            mSelectedEntities.insert(entities[i]);
                    }
                    else
                    {
                        for (size_t i = currentIndex; i <= lastSelectedIndex; ++i)
                            mSelectedEntities.insert(entities[i]);
                    }
                }
            }

            // Add context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    for (Gep::Entity selectedEntity : mSelectedEntities)
                    {
                        mManager.MarkEntityForDestruction(selectedEntity);
                    }
                    mSelectedEntities.clear();
                }
                if (ImGui::MenuItem("Duplicate"))
                {
                    for (Gep::Entity selectedEntity : mSelectedEntities)
                    {
                        mManager.DuplicateEntity(selectedEntity);
                    }
                }
                ImGui::EndPopup();
            }

            // Add drag and drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                std::vector<Gep::Entity> selectedEntities(mSelectedEntities.begin(), mSelectedEntities.end());

                ImGui::SetDragDropPayload("ENTITY", selectedEntities.data(), selectedEntities.size() * sizeof(Gep::Entity));
                ImGui::Text("Dragging %s", displayName.c_str());
                ImGui::EndDragDropSource();
            }

            // Add drag and drop target
            EntitiesDragDropTarget([&](Gep::Entity droppedEntity)
            {
                if (droppedEntity == entity) return;

                mManager.AttachEntity(entity, droppedEntity);
            });

            if (isOpen)
            {
                DrawEntities(mManager.GetChildren(entity), dt);

                ImGui::TreePop();
            }

            ImGui::PopID();
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
