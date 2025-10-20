/*****************************************************************//**
 * \file   ImGuiSystem.cpp
 * \brief  implementation of the ImGuiSystem
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "ImGuiSystem.hpp"

#include "CameraComponent.hpp"
#include "Transform.hpp"
#include "ModelComponent.hpp"
#include "CubeCollider.hpp"
#include "SphereCollider.hpp"
#include "LightComponent.hpp"

#include "imgui_te_engine.h"
 //#include "ImGuizmo.h"
#include "SerializationResource.hpp"
#include "EditorResource.hpp"

#include "OS.hpp"

namespace Client
{
    ImGuiSystem::ImGuiSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mEditorResource(em.GetResource<EditorResource>())
    {
    }

    std::string ImGuiSystem::GetEntityDisplayName(Gep::Entity entity)
    {
        std::string name = mManager.GetName(entity);
        if (!name.empty())
        {
            return name;
        }

        return "Entity: " + std::to_string(entity);
    }

    void ImGuiSystem::DrawInspectorPanel()
    {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

        // case where there is no entities selected
        if (mEditorResource.mSelectedEntities.size() == 0)
        {
            ImGui::Text("No Entity Selected");
            ImGui::End(); // Inspector
            return;
        }

        // case where there is multiple entities selected, display the names of each selected entity
        if (mEditorResource.mSelectedEntities.size() > 1)
        {
            ImGui::Text("%d Entities Selected", mEditorResource.mSelectedEntities.size());

            ImGui::Separator();

            for (Gep::Entity entity : mEditorResource.mSelectedEntities)
            {
                std::string displayName = GetEntityDisplayName(entity);
                ImGui::Text(displayName.c_str());

                if (mManager.HasComponent<ModelComponent>(entity))
                {
                    Client::ModelComponent& material = mManager.GetComponent<Client::ModelComponent>(entity);
                    material.selected = true;
                }
            }

            std::string name = mManager.GetName(*mEditorResource.mSelectedEntities.begin());
            if (ImGui::InputText("Batch Rename", &name))
            {
                for (Gep::Entity entity : mEditorResource.mSelectedEntities)
                {
                    mManager.SetName(entity, name);
                }
            }
            ImGui::End(); // Inspector
            return;
        }

        // case where there is only a single entity selected, display its name and all of its components
        Gep::Entity entity = *mEditorResource.mSelectedEntities.begin();
        std::string displayName = mManager.GetName(entity);
        if (ImGui::InputText("Name", &displayName))
        {
            mManager.SetName(entity, displayName);
        }
        ImGui::Text("Name: %s", displayName.c_str());
        ImGui::Text("RTID: %u", entity);
        ImGui::Text("UUID: %s", mManager.GetUUID(entity).ToString().c_str());

        ImGui::Dummy({ 0.0f, 10.0f });

        if (mManager.HasComponent<ModelComponent>(entity))
        {
            Client::ModelComponent& material = mManager.GetComponent<Client::ModelComponent>(entity);
            material.selected = true;
        }

        // display the components imgui dropdown
        mManager.ForEachComponent(entity, [&](const Gep::ComponentData& componentData)
        {
            mComponentInspectorPanels[componentData.index](entity);
        });

        ImGui::Dummy({ 0.0f, 10.0f });

        ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        ImGui::PushStyleColor(ImGuiCol_Header, buttonColor);

        // dropdown at the bottom of an entities panel that allows adding of components
        if (ImGui::CollapsingHeader("Add Component"))
        {
            // iterate over the components that an entity DOESN'T have
            for (const auto& [index, componentData] : mManager.GetComponentDatas())
            {
                if (componentData.has(entity)) continue;

                if (ImGui::Button(componentData.name.c_str(), { ImGui::GetContentRegionAvail().x, 0.0f }))
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

        // performance information
        ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("Latency: %.2f ms", ImGui::GetIO().DeltaTime * 1000.0f);
        ImGui::Separator();

        // entity information
        ImGui::Text("Entities: %d", mManager.GetEntities().size());
        ImGui::Separator();

        // component information
        const auto& components = mManager.GetComponentDatas();
        ImGui::Text("Components Registered: %u", components.size());

        if (ImGui::BeginTable("ComponentTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            for (const auto& [index, component] : components)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", component.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%u", component.count);
                ImGui::TableNextColumn();
                ImGui::Text("%u", component.index);
                ImGui::TableNextColumn();
                ImGui::Text("%u", component.size);
            }

            ImGui::EndTable();
        }
        ImGui::Separator();

        // archetype information
        const auto& archetypes = mManager.GetArchetypes();
        ImGui::Text("Active Archetypes: %u", archetypes.size());
        if (ImGui::BeginTable("ArchetypeTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Archetype", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Stride", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            for (const auto& [signature, chunk] : archetypes)
            {
                ImGui::TableNextRow();
                std::string archetypeContents = "<";
                mManager.ForEachComponentBit(signature, [&](const Gep::ComponentData& data)
                {
                    archetypeContents += data.name + ", ";
                });
                archetypeContents.pop_back(); // remove the space
                archetypeContents.back() = '>'; // replace the comma

                ImGui::TableNextColumn();
                ImGui::Text("%s", archetypeContents.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%u", chunk.entityCount);
                ImGui::TableNextColumn();
                ImGui::Text("%u", chunk.stride);
            }

            ImGui::EndTable();
        }
        ImGui::Separator();

        const auto& systems = mManager.GetSystemDatas();
        ImGui::Text("Active Systems: %u", systems.size());
        if (ImGui::BeginTable("SystemTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("System", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Initialize", ImGuiTableColumnFlags_WidthFixed);
            //ImGui::TableSetupColumn("Frame Start", ImGuiTableColumnFlags_WidthFixed);
            //ImGui::TableSetupColumn("Update", ImGuiTableColumnFlags_WidthFixed);
            //ImGui::TableSetupColumn("Frame End", ImGuiTableColumnFlags_WidthFixed);
            //ImGui::TableSetupColumn("Exit", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Frame %", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            for (const auto& [index, data] : systems)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", data.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%u", data.index);
                ImGui::TableNextColumn();
                ImGui::Text("%u", data.size);
                ImGui::TableNextColumn();
                ImGui::Text("%.3fms", data.timeInInitialize * 1000.0f);
                //ImGui::TableNextColumn();
                //ImGui::Text("%.3fms", data.timeInFrameStart * 1000.0f);
                //ImGui::TableNextColumn();
                //ImGui::Text("%.3fms", data.timeInUpdate * 1000.0f);
                //ImGui::TableNextColumn();
                //ImGui::Text("%.3fms", data.timeInFrameEnd * 1000.0f);
                //ImGui::TableNextColumn();
                //ImGui::Text("%.3fms", data.timeInExit * 1000.0f);

                float engineFrameTime = mManager.GetDeltaTime();
                float systemFrameTime = data.timeInFrameStart + data.timeInUpdate + data.timeInFrameEnd;

                ImGui::TableNextColumn();
                ImGui::Text("%.1f%", (systemFrameTime / engineFrameTime) * 100.0f);
            }

            ImGui::EndTable();
        }
        ImGui::Separator();


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
        ImVec4 color = mEditorResource.mSelectedEntities.contains(entity) ? selectedColor : defaultColor;
        ImVec4 hoverColor = ImVec4(color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hoverColor);
        ImGui::PushStyleColor(ImGuiCol_Header, mEditorResource.mSelectedEntities.contains(entity) ? selectedColor : defaultColor);

        bool isOpen = ImGui::TreeNodeEx(displayName.c_str()
            , ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | (mManager.HasChild(entity) ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf)
            | (mEditorResource.mSelectedEntities.contains(entity) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None));

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        return isOpen;
    }

    void ImGuiSystem::DrawQuickTest()
    {
    }

    std::vector<Gep::Entity> ImGuiSystem::SearchEntities(const std::vector<Gep::Entity>& entities, const std::string& searchTerm)
    {
        std::vector<Gep::Entity> result;

        // lowers, then splits up the search term by spaces
        std::string lowerSearchTerm = searchTerm;
        Gep::ToLower(lowerSearchTerm);

        for (Gep::Entity entity : entities)
        {
            std::string displayName = GetEntityDisplayName(entity);
            Gep::ToLower(displayName);

            // extracts the current search term
            std::istringstream iss(lowerSearchTerm);
            std::string currentSearchTerm;
            bool matchesAll = true;
            while (iss >> currentSearchTerm)
            {
                // if the entities name matchs ALL of the search terms, add it only then
                if (displayName.find(currentSearchTerm) == std::string::npos)
                {
                    matchesAll = false;
                }
            }
            if (matchesAll)
            {
                result.push_back(mManager.GetRoot(entity));
            }
        }


        // remove duplicates
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());

        return result;
    }

    void ImGuiSystem::SetAssetBrowserPath(const std::filesystem::path& newPath)
    {
        if (!std::filesystem::exists(mAssetBrowserPath))
        {
            mAssetBrowserPath = std::filesystem::current_path();
            Gep::Log::Error("Path given doesn't exist: [", newPath, "]. Using: [", mAssetBrowserPath, "] instead.");
        }

        mAssetBrowserPath = newPath;
        mAssetBrowserEntries.clear();

        for (const auto& entry : std::filesystem::directory_iterator(mAssetBrowserPath))
        {
            mAssetBrowserEntries.push_back(entry);
        }
    }

    void ImGuiSystem::ReloadAssetBrowser()
    {
        SetAssetBrowserPath(mAssetBrowserPath);
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
                mEditorResource.mSelectedEntities.clear();

            }
            ImGui::EndDragDropTarget();
        }
    }

    void ImGuiSystem::OnEntityCreated(const Gep::Event::EntityCreated& event)
    {
        mEditorResource.mHierarchyEntities.push_back(event.entity);
    }

    void ImGuiSystem::OnEntityDestroyed(const Gep::Event::EntityDestroyed& event)
    {
        mEditorResource.mSelectedEntities.erase(event.entity);

        // removes the entity from the heirarchy
        auto& es = mEditorResource.mHierarchyEntities;
        es.erase(std::remove(es.begin(), es.end(), event.entity), es.end());
    }

    void ImGuiSystem::OnMouseScrolled(const Gep::Event::MouseScrolled& event)
    {
        static float scale = 1.0f;

        // glfw check if lcrtl is pressed
        if (ImGui::GetIO().KeyCtrl)
        {
            scale += event.yoffset * 0.1f;
            scale = glm::clamp(scale, 0.1f, 10.0f);
            ImGui::GetIO().FontGlobalScale = scale;
        }
    }

    void ImGuiSystem::OnFileDropped(const Gep::Event::FileDropped& event)
    {
        // checks if there are and duplicates, if there are any cancel the copy
        for (const auto& path : event.droppedFiles)
        {
            auto it = std::find_if(mAssetBrowserEntries.begin(), mAssetBrowserEntries.end(), [path](const auto& entry)
            {
                return path.filename() == entry.path().filename();
            });

            if (it != mAssetBrowserEntries.end())
            {
                Gep::Log::Error("Cannot drop files, the file: ", path.filename().string(), " already exists in the current working directory. No files were copied.");
                return;
            }
        }

        for (const auto& path : event.droppedFiles)
        {
            const std::filesystem::path destination = mAssetBrowserPath / path.filename();
            std::filesystem::copy(path, destination, std::filesystem::copy_options::recursive);
        }

        ReloadAssetBrowser();
    }

    void ImGuiSystem::OnEntityAttached(const Gep::Event::EntityAttached& event)
    {

    }

    void ImGuiSystem::OnEntityDetached(const Gep::Event::EntityDetached& event)
    {
    }

    void ImGuiSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::MouseScrolled>(this, &ImGuiSystem::OnMouseScrolled);
        mManager.SubscribeToEvent<Gep::Event::EntityDestroyed>(this, &ImGuiSystem::OnEntityDestroyed);
        mManager.SubscribeToEvent<Gep::Event::EntityCreated>(this, &ImGuiSystem::OnEntityCreated);
        mManager.SubscribeToEvent<Gep::Event::FileDropped>(this, &ImGuiSystem::OnFileDropped);
        mManager.SubscribeToEvent<Gep::Event::EntityAttached>(this, &ImGuiSystem::OnEntityAttached);
        mManager.SubscribeToEvent<Gep::Event::EntityDetached>(this, &ImGuiSystem::OnEntityDetached);

        mAssetBrowserPath = std::filesystem::current_path() / "assets";

        for (const auto& entry : std::filesystem::directory_iterator(mAssetBrowserPath))
        {
            mAssetBrowserEntries.push_back(entry);
        }

        // auto generated by ImGui Style
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.09f, 0.09f, 0.09f, 0.44f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.02f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        ImGui::GetStyle().WindowRounding = 0;
        ImGui::GetStyle().ChildRounding = 0;
        ImGui::GetStyle().FrameRounding = 0;
        ImGui::GetStyle().PopupRounding = 0;
        ImGui::GetStyle().ScrollbarRounding = 0;
        ImGui::GetStyle().GrabRounding = 0;
        ImGui::GetStyle().TabRounding = 0;

        ImGui::GetStyle().DockingSeparatorSize = 6;
        ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
    }

    void ImGuiSystem::Update(float dt)
    {
        DrawInfoPanel();
        DrawExtras();
        DrawToolbar();
        DrawQuickTest();

        ImGui::Begin("Entities");

        // search bar
        static std::string search = "";

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false))
        {
            ImGui::SetKeyboardFocusHere();
        }
        //ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("###Search", "Search", &search);

        auto& heirarchyEnts = mEditorResource.mHierarchyEntities;

        std::vector<Gep::Entity> roots;
        if (search.empty())
        {
            for (Gep::Entity entity : heirarchyEnts)
            {
                if (!mManager.HasParent(entity))
                {
                    roots.push_back(entity);
                }
            }
        }
        else
        {
            roots = SearchEntities(heirarchyEnts, search);
        }

        ImGui::SameLine();
        if (ImGui::BeginCombo("Sort", "", ImGuiComboFlags_::ImGuiComboFlags_NoPreview))
        {
            if (ImGui::Selectable("Name"))
            {
                std::sort(heirarchyEnts.begin(), heirarchyEnts.end(), [&](Gep::Entity a, Gep::Entity b)
                {
                    const std::string& aName = mManager.GetName(a);
                    const std::string& bName = mManager.GetName(b);
                    return aName < bName;  // ascending order
                });
            }
            if (ImGui::Selectable("RTID"))
            {
                std::sort(heirarchyEnts.begin(), heirarchyEnts.end(), [&](Gep::Entity a, Gep::Entity b)
                {
                    return a < b;
                });
            }
            if (ImGui::Selectable("UUID"))
            {
                std::sort(heirarchyEnts.begin(), heirarchyEnts.end(), [&](Gep::Entity a, Gep::Entity b)
                {
                    const std::string& aUUID = mManager.GetUUID(a).ToString();
                    const std::string& bUUID = mManager.GetUUID(b).ToString();
                    return aUUID < bUUID;  // descending order
                });
            }
            ImGui::EndCombo();
        }


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("Entities");
        DrawEntities(roots, dt);
        // detach entities if they are dropped into any open space, adds a little bit of extra dropping space aswell
        ImVec2 size = ImGui::GetContentRegionAvail();
        size.y = 200 * ImGui::GetIO().FontGlobalScale;
        ImGui::Dummy(size);

        EntitiesDragDropTarget([&](Gep::Entity entity)
        {
            mManager.DetachEntity(entity);
        });

        // clears selected entities if the background is clicked
        if (ImGui::IsItemClicked())
        {
            mEditorResource.mSelectedEntities.clear();
        }
        ImGui::EndChild();


        // delete selected entities
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        {
            for (Gep::Entity selectedEntity : mEditorResource.mSelectedEntities)
            {
                mManager.MarkEntityForDestruction(selectedEntity);
            }
            mEditorResource.mSelectedEntities.clear();
        }

        // duplicate selected entities
        if (ImGui::IsKeyPressed(ImGuiKey_D, false) && ImGui::GetIO().KeyCtrl)
        {
            for (Gep::Entity selectedEntity : mEditorResource.mSelectedEntities)
            {
                mManager.DuplicateEntity(selectedEntity);
            }
        }

        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_A, false) && ImGui::GetIO().KeyCtrl)
        {
            mEditorResource.mSelectedEntities.clear();
            for (Gep::Entity entity : mEditorResource.mHierarchyEntities)
            {
                mEditorResource.mSelectedEntities.insert(entity);
            }
        }

        DrawInspectorPanel();
        DrawAssetBrowser();

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

            // default selection
            if (ImGui::IsItemClicked() && !isCtrlPressed && !isShiftPressed && mEditorResource.mSelectedEntities.size() <= 1)
            {
                mEditorResource.SelectEntity(entity);
                mEditorResource.mLastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
            }

            // same as default but if there are multiple entities selected, doesn't deselect until released
            else if (ImGui::IsItemDeactivated() && !isCtrlPressed && !isShiftPressed && mEditorResource.mSelectedEntities.size() > 1)
            {
                mEditorResource.SelectEntity(entity);
                mEditorResource.mLastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
            }

            // will select the entity if it is clicked, and unselect if it is clicked again
            else if (ImGui::IsItemClicked() && isCtrlPressed && !isShiftPressed)
            {
                if (mEditorResource.IsEntitySelected(entity))
                    mEditorResource.DeselectEntity(entity);
                else
                {
                    mEditorResource.SelectAnotherEntity(entity);
                    mEditorResource.mLastSelectedIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
                }
            }

            // multiselect with shift key
            else if (ImGui::IsItemClicked() && !isCtrlPressed && isShiftPressed)
            {
                mEditorResource.mSelectedEntities.clear();
                if (mEditorResource.mLastSelectedIndex < entities.size())
                {
                    size_t currentIndex = std::distance(entities.cbegin(), std::find(entities.cbegin(), entities.cend(), entity));
                    if (mEditorResource.mLastSelectedIndex < currentIndex)
                    {
                        for (size_t i = mEditorResource.mLastSelectedIndex; i <= currentIndex; ++i)
                            mEditorResource.SelectAnotherEntity(entities[i]);
                    }
                    else
                    {
                        for (size_t i = currentIndex; i <= mEditorResource.mLastSelectedIndex; ++i)
                            mEditorResource.SelectAnotherEntity(entities[i]);
                    }
                }
            }

            // right click menu, with delete and duplicate, etc options
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    for (Gep::Entity selectedEntity : mEditorResource.mSelectedEntities)
                    {
                        mManager.MarkEntityForDestruction(selectedEntity);
                    }
                    mEditorResource.mSelectedEntities.clear();
                }
                if (ImGui::MenuItem("Duplicate"))
                {
                    for (Gep::Entity selectedEntity : mEditorResource.mSelectedEntities)
                    {
                        mManager.DuplicateEntity(selectedEntity);
                    }
                }

                if (mEditorResource.mSelectedEntities.size() == 1)
                {
                    Gep::Entity selectedEntity = *mEditorResource.mSelectedEntities.begin();
                    if (ImGui::MenuItem("Save as prefab"))
                    {
                        auto& sr = mManager.GetResource<Client::SerializationResource>();
                        sr.SavePrefab(mManager.SaveEntity(selectedEntity), "assets\\prefabs\\" + GetEntityDisplayName(selectedEntity) + ".prefab");
                    }
                }
                ImGui::EndPopup();
            }

            // drag and drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                std::vector<Gep::Entity> selectedEntities(mEditorResource.mSelectedEntities.begin(), mEditorResource.mSelectedEntities.end());

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
    void ImGuiSystem::DrawAssetBrowser()
    {
        Gep::OpenGLRenderer& renderer = mManager.GetResource<Gep::OpenGLRenderer>();

        ImGui::GetCurrentContext()->IO.ConfigDockingAlwaysTabBar = true;
        ImGui::Begin("Asset Browser", nullptr, ImGuiWindowFlags_NoCollapse);

        static const std::filesystem::path workingDir = std::filesystem::current_path();

        const float imageSize = 64.0f * ImGui::GetIO().FontGlobalScale;
        const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        const int imagesPerRow = static_cast<int>((contentRegion.x - ImGui::GetStyle().ScrollbarSize) / (imageSize + spacing));

        if (imagesPerRow < 1)
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Back"))
        {
            SetAssetBrowserPath(mAssetBrowserPath.parent_path());
        }
        ImGui::SameLine();
        ImGui::Text("%s", mAssetBrowserPath.string().c_str());


        ImGui::BeginChild("AssetGrid");

        for (size_t i = 0; i < mAssetBrowserEntries.size(); ++i)
        {
            const auto& entry = mAssetBrowserEntries[i];
            const std::string filename = entry.path().filename().string();
            const std::string filenameButHidden = "##" + filename;
            const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            const std::filesystem::path relativePath = entry.path().lexically_relative(workingDir);
            const Gep::Texture texture = renderer.GetOrLoadIconTexture(relativePath);

            constexpr float imageToTextDistance = 6.0f;

            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginGroup();

            // first draws an image for the thumpnail of the asset
            ImGui::Image((ImTextureID)texture.id, ImVec2(imageSize, imageSize));

            // next draws the text under the file image
            ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + imageSize + imageToTextDistance));//
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + imageSize); // wrap at imageSize pixels
            ImGui::TextWrapped("%s", filename.c_str());
            ImGui::PopTextWrapPos();

            // gets the size of the text after the wrapping is calculated
            ImVec2 textSize = ImGui::GetItemRectSize();

            // setsup the style of the button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.2));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.3));

            ImGui::SetCursorScreenPos(cursorPos);
            ImGui::Button(filenameButHidden.c_str(), ImVec2(imageSize, imageSize + imageToTextDistance + textSize.y));
            ImGui::PopStyleColor(4);
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                // Set the payload to carry the relative path
                std::string pathStr = relativePath.string();
                ImGui::SetDragDropPayload("ASSET_BROWSER", pathStr.c_str(), pathStr.size() + 1);

                // Display the dragged item
                ImGui::Image((void*)(intptr_t)texture.id, { imageSize * 2.0f, imageSize * 2.0f });
                ImGui::TextWrapped("%s", entry.path().filename().string().c_str());
                ImGui::EndDragDropSource();
            }

            mEditorResource.AssetBrowserDropTarget([&](const std::filesystem::path& droppedPath)
            {
                if (entry.is_directory())
                {
                    std::filesystem::path target = std::filesystem::current_path() / droppedPath;
                    std::filesystem::path destination = entry.path() / droppedPath.filename();
                    std::filesystem::rename(target, destination);
                    ReloadAssetBrowser();
                }
            });

            if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) 
            {
                if (entry.is_directory())
                {
                    SetAssetBrowserPath(entry.path());
                    ImGui::EndGroup();
                    ImGui::PopID();
                    break;
                }

                mManager.SignalEvent(Gep::Event::AssetBrowserItemClicked{ entry.path(), entry.path().extension().string() });
            }

            ImGui::EndGroup();
            ImGui::PopID();

            // Move to next column
            if ((i + 1) % imagesPerRow == 0) 
            {
                ImGui::NewLine();
            }
            else 
            {
                ImGui::SameLine();
            }
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // Global state for pause/play
    bool paused = false;

    void ImGuiSystem::ShowControlBar()
    {
        // Create a window without a title bar, resize, or scrollbar.
        ImGui::Begin("ControlBar", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        // Toggle pause/play button.
        if (paused)
        {
            if (ImGui::Button("Play"))
            {
                paused = false;
            }
        }
        else
        {
            if (ImGui::Button("Pause"))
            {
                paused = true;
            }
        }

        ImGui::End();
    }

    void ImGuiSystem::DrawToolbar()
    {
        if (ImGui::BeginMainMenuBar()) // Creates a top menu bar
        {
            if (ImGui::BeginMenu("File"))
            {
                Client::SerializationResource& sr = mManager.GetResource<Client::SerializationResource>();

                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    std::filesystem::path newSceneFilePath = Gep::DialogBox_SaveAs("assets\\scenes", "Scene File", "scene", "New Scene");

                    sr.NewScene(newSceneFilePath);
                }
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    sr.SaveScene(mManager);
                }
                if (ImGui::MenuItem("Save As", "Ctrl+Alt+S"))
                {
                    std::filesystem::path newSceneFilePath = Gep::DialogBox_SaveAs("assets\\scenes", "Scene File", "scene", "New Scene");

                    sr.SaveScene(mManager, newSceneFilePath);
                }
                if (ImGui::MenuItem("Open project in explorer"))
                {
                    Gep::OpenInExplorer(std::filesystem::current_path());
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Ctrl+Alt+Esc"))
                {
                    mManager.Shutdown();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Empty"))
                { 
                    Gep::Entity entity = mManager.CreateEntity("Empty");
                    mEditorResource.SelectEntity(entity);
                }
                if (ImGui::MenuItem("Cube"))
                {
                    Gep::Entity entity = mManager.CreateEntity("Cube");
                    mManager.AddComponent(entity, ModelComponent{ "Cube" }
                                                , Transform{}
                                                , CubeCollider{});
                    mEditorResource.SelectEntity(entity);
                }
                if (ImGui::MenuItem("Sphere"))
                {
                    Gep::Entity entity = mManager.CreateEntity("Icosphere");
                    mManager.AddComponent(entity, ModelComponent{ "Icosphere" }
                                                , Transform{}
                                                , SphereCollider{});
                    mEditorResource.SelectEntity(entity);
                }
                if (ImGui::MenuItem("Light"))
                {
                    Gep::Entity entity = mManager.CreateEntity("Light");
                    mManager.AddComponent(entity, ModelComponent{ "Sphere" }
                                                , Transform{}
                                                , Light{}
                                                , SphereCollider{});
                    mEditorResource.SelectEntity(entity);
                }
                if (ImGui::MenuItem("Camera"))
                {
                    Gep::Entity entity = mManager.CreateEntity("Camera");
                    mManager.AddComponent(entity, Camera{}
                                                , Transform{});
                    mEditorResource.SelectEntity(entity);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                //if (ImGui::MenuItem("Undo")) { /* Handle undo */ }
                //if (ImGui::MenuItem("Redo")) { /* Handle redo */ }
                //ImGui::Separator();
                //if (ImGui::MenuItem("Cut")) { /* Handle cut */ }
                //if (ImGui::MenuItem("Copy")) { /* Handle copy */ }
                //if (ImGui::MenuItem("Paste")) { /* Handle paste */ }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows"))
            {
                static bool showConsole = true;
                static bool showInspector = true;

                if (ImGui::MenuItem("Console", nullptr, &showConsole)) { /* Toggle Console */ }
                if (ImGui::MenuItem("Inspector", nullptr, &showInspector)) { /* Toggle Inspector */ }
                ImGui::EndMenu();
            }

            static std::string label = "Play";

            // Size for buttons
            // This matches what ImGui::Button does internally
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 text_size = ImGui::CalcTextSize(label.c_str(), nullptr, true);
            ImVec2 buttonSize = ImVec2(
                text_size.x + style.FramePadding.x * 2.0f,
                text_size.y + style.FramePadding.y * 2.0f
            );            

            // Calculate how much space the two buttons will take
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float totalWidth = (buttonSize.x * 2) + spacing;
            float menuBarWidth = ImGui::GetWindowWidth();
            float offsetX = (menuBarWidth - totalWidth) * 0.5f;

            // Move cursor to centered position
            ImGui::SetCursorPosX(offsetX);

            bool wasEditing = mManager.IsState(Gep::EngineState::Edit);
            // Toggle play/pause
            bool isPlaying = mManager.IsState(Gep::EngineState::Play);
            label = isPlaying ? "Pause" : "Play";
            if (ImGui::Button(label.c_str(), buttonSize))
            {
                if (isPlaying)
                    mManager.SetState(Gep::EngineState::Pause);
                else
                    mManager.SetState(Gep::EngineState::Play);

                if (wasEditing)
                {
                    mManager.GetResource<SerializationResource>().SaveScene(mManager);
                    mManager.GetResource<SerializationResource>().ReloadScene(mManager);
                }
            }

            ImGui::SameLine();

            // stop only works if in game
            if (ImGui::Button("Stop")) 
            {
                if (!mManager.IsState(Gep::EngineState::Edit))
                {
                    auto& scr = mManager.GetResource<SerializationResource>();

                    mManager.DestroyAllEntities();
                    mManager.SetState(Gep::EngineState::Edit);
                    scr.LoadScene(mManager, scr.currentScenePath);
                }
            }
            ImGui::EndMainMenuBar();
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
