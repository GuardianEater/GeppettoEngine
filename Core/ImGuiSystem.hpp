/*****************************************************************//**
 * \file   ImGuiSystem.hpp
 * \brief  System that operates imgui
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

 // backend
#include <imgui.h>
#include <imgui_stdlib.h>
#include <ISystem.hpp>
#include "Identification.hpp"

#include <rfl.hpp>

#include "Logger.hpp"
#include "TypeID.hpp"

// client



namespace Client
{
    // Concept to check if a type has an iterator
    template <typename T>
    concept TypeIsContainer = requires(T t)
    {
        typename T::iterator;
        typename T::value_type;

        { t.begin() } -> std::same_as<typename T::iterator>;
        { t.emplace_back() };
        { t.end() } -> std::same_as<typename T::iterator>;
    };

    // Concept to check if a type has a value_type
    template <typename T>
    concept HasValueType = requires(T t)
    {
        typename T::value_type;
    };

    class ImGuiSystem : public Gep::ISystem
    {
    public:
        std::unordered_map<Gep::Signature, std::function<void(Gep::Entity)>> mComponentFunctions;

        ImGuiSystem(Gep::EngineManager& em)
            : ISystem(em)
        {
        }

        template <typename... ComponentTypes>
        void OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes)
        {
            componentTypes.for_each([&]<typename ComponentType>()
            {
                mComponentFunctions[mManager.GetComponentSignature<ComponentType>()] = [&](Gep::Entity entity)
                {
                    if (ImGui::CollapsingHeader(Gep::GetTypeInfo<ComponentType>().PrettyName().c_str()))
                    {
                        ComponentType& component = mManager.GetComponent<ComponentType>(entity);

                        const auto view = rfl::to_view(component);

                        view.apply([&](const auto& f) 
                        {
                            DrawType(f.name(), *f.value());
                        });
                    }
                };
            });
        }

        void Update(float dt) override
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

            DrawEntities(parents);

            ImGui::End();
        }

        void DrawEntities(const std::vector<Gep::Entity>& entities)
        {
            for (Gep::Entity entity : entities)
            {
                std::string displayName = "Entity: " + std::to_string(entity);
                if (mManager.HasComponent<Identification>(entity))
                {
                    displayName = mManager.GetComponent<Identification>(entity).name;
                }

                displayName += "###" + std::to_string(entity);
                if (ImGui::TreeNodeEx(displayName.c_str()))
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

        template <typename T>
        void DrawType(const std::string_view name, T& t)
        {
            std::string typeName = Gep::GetTypeInfo<T>().PrettyName();
            ImGui::Text(typeName.c_str());
        }

        template <typename T>
        requires TypeIsContainer<T>
        void DrawType(const std::string_view name, T& t)
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

        template <>
        void DrawType<float>(const std::string_view name, float& t)
        {
            ImGui::DragFloat(name.data(), &t);
        }

        template <>
        void DrawType<int>(const std::string_view name, int& t)
        {
            ImGui::InputInt(name.data(), &t);
        }

        template <>
        void DrawType<std::string>(const std::string_view name, std::string& t)
        {
            ImGui::InputText(name.data(), &t);
        }

        template <>
        void DrawType<glm::vec3>(const std::string_view name, glm::vec3& t)
        {
            ImGui::DragFloat3(name.data(), &t.x);
        }

        template <>
        void DrawType<glm::vec4>(const std::string_view name, glm::vec4& t)
        {
            ImGui::DragFloat4(name.data(), &t.x);
        }

        template <>
        void DrawType<size_t>(const std::string_view name, size_t& t)
        {
            ImGui::InputScalar(name.data(), ImGuiDataType_U64, &t);
        }

        template <>
        void DrawType<bool>(const std::string_view name, bool& t)
        {
            ImGui::Checkbox(name.data(), &t);
        }

        template <>
        void DrawType<glm::quat>(const std::string_view name, glm::quat& t)
        {
            ImGui::DragFloat4(name.data(), &t.x);
        }

        template <>
        void DrawType<char>(const std::string_view name, char& t)
        {
            ImGui::InputScalar(name.data(), ImGuiDataType_S8, &t);
        }

    };
}
