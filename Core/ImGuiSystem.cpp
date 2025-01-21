/*****************************************************************//**
 * \file   ImGuiSystem.cpp
 * \brief  implementation of the ImGuiSystem
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#include "ImGuiSystem.hpp"

namespace Client
{
    ImGuiSystem::ImGuiSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
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

        if (ImGui::Button("Create"))
        {
            mManager.CreateEntity();
        }

        DrawEntities(parents);

        ImGui::End(); // Entities
    }

    void ImGuiSystem::DrawEntities(const std::vector<Gep::Entity>& entities)
    {
        for (Gep::Entity entity : entities)
        {
            std::string displayName = "Entity: " + std::to_string(entity);
            if (mManager.HasComponent<Identification>(entity))
            {
                Identification& id = mManager.GetComponent<Identification>(entity);

                if (!id.name.empty())
                    displayName = id.name;
            }

            displayName += "###" + std::to_string(entity);

            ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // Increase padding
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f)); // Increase vertical padding
            bool open = ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding);

            // Add context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    mManager.MarkEntityForDestruction(entity);
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


            ImGui::PopStyleVar(2);

            if (open)
            {
                ImGui::Begin(std::string("Inspector: " + displayName).c_str());

                std::vector<Gep::Signature> componentSignatures = mManager.GetComponentSignatures(entity);

                for (const Gep::Signature componentSignature : componentSignatures)
                {
                    mComponentFunctions[componentSignature](entity);
                }

                ImGui::End(); // Inspector
                DrawEntities(mManager.GetChildren(entity));
                ImGui::TreePop();
            }

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
