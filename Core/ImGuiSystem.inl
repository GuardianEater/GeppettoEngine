/*****************************************************************//**
 * \file   ImGuiSystem.inl
 * \brief  
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "ImGuiSystem.hpp"
#include "Events.hpp"
namespace Client
{
    template <typename ComponentType>
    concept HasOnImGuiRender = requires(ComponentType component)
    {
        { component.OnImGuiRender() } -> std::same_as<void>;
    };

    template <typename ComponentType>
    concept HasOnImGuiRenderWithManager = requires(ComponentType component, Gep::EngineManager& manager)
    {
        { component.OnImGuiRender(manager) } -> std::same_as<void>;
    };


    template <typename... ComponentTypes>
    void ImGuiSystem::OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes)
    {
        componentTypes.for_each([&]<typename ComponentType>()
        {
            mComponentInspectorPanels.push_back([&](std::span<Gep::Entity> entities)
            {
                std::string componentName = Gep::GetTypeInfo<ComponentType>().PrettyName();

                bool open = ImGui::CollapsingHeader(componentName.c_str());

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Delete"))
                    {
                        for (auto entity : entities)
                            mManager.DestroyComponent<ComponentType>(entity);
                    }
                    ImGui::EndPopup();
                    return;
                }

                if (open)
                {
                    // the entitiy must have the component, because the component count and entity count are must be the same
                    std::vector<ComponentType*> components;
                    for (auto entity : entities)
                        components.push_back(&mManager.GetComponent<ComponentType>(entity));

                    mManager.SignalEvent(Gep::Event::ComponentEditorRender<ComponentType>({ entities, components }));

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }
            });
        });
    }

    template <typename T>
    void ImGuiSystem::DrawType(const std::string_view name, T& t)
    {
        std::string typeName = Gep::GetTypeInfo<T>().PrettyName();
        ImGui::Text(typeName.c_str());
    }

    template <typename T>
    requires TypeIsContainer<T>
    void ImGuiSystem::DrawType(const std::string_view name, T& t)
    {
        size_t index = 0;
        if (ImGui::TreeNode(name.data()))
        {
            for (auto it = t.begin(); it != t.end(); ++it)
            {
                std::string itemName = name.data() + std::string("[") + std::to_string(index++) + "]";
                DrawType(itemName, *it);
            }

            if constexpr (std::is_default_constructible_v<typename T::value_type>)
            {
                if (ImGui::Button(" + "))
                {
                    t.emplace_back();
                }
                ImGui::SameLine();
                if (ImGui::Button(" - "))
                {
                    if (!t.empty())
                        t.pop_back();
                }
            }

            ImGui::TreePop();
        }
    }
}
