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

#include <glm.hpp>

#include "Logger.hpp"
#include "TypeID.hpp"
#include "TypeList.hpp"
#include "EngineManager.hpp"

// client

namespace Client
{
    // forward declaration
    class EditorResource;

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
    private:
        std::string GetEntityDisplayName(Gep::Entity entity);
        void DrawInspectorPanel();
        void DrawInfoPanel();
        void DrawExtras();
        bool DrawEntityNode(Gep::Entity entity);

        // calls the given function for each entity dropped
        template <typename FunctionType>
        requires std::invocable<FunctionType, Gep::Entity>
        void EntitiesDragDropTarget(FunctionType func);

        
        void StartCameraFocus(Gep::Entity entity); // sets the target that the camera will look at
        void UpdateCameraFocus(float dt); // lerps the camera to the target entity set with StartCameraFocus
        bool mCameraLerping = false;
        glm::vec3 mCameraTargetRotation{};
        glm::vec3 mCameraTargetPosition{};

        std::vector<Gep::Entity> SearchEntities(const std::vector<Gep::Entity>& entities, const std::string& searchTerm);

    private:
        std::vector<Gep::Entity> mEntities;
        EditorResource& mEditorResource;

        std::vector<std::function<void(Gep::Entity)>> mComponentInspectorPanels; // component index -> function to draw the inspector

    public:

        ImGuiSystem(Gep::EngineManager& em);

        template <typename... ComponentTypes>
        void OnComponentsRegistered(Gep::type_list<ComponentTypes...> componentTypes);

        void OnEntityDestroyed(const Gep::Event::EntityDestroyed& event);
        void OnMouseScrolled(const Gep::Event::MouseScrolled& event);

        void Initialize() override;
        void Update(float dt) override;

        void DrawEntities(const std::vector<Gep::Entity>& entities, float dt);
        void DrawAssetBrowser();
        void DrawToolbar();
        void ShowControlBar();

        template <typename T>
        void DrawType(const std::string_view name, T& t);

        template <typename T>
        requires TypeIsContainer<T>
        void DrawType(const std::string_view name, T& t);

        // bunch of base type specializations
        template <> void DrawType<float>(const std::string_view name, float& t);
        template <> void DrawType<int>(const std::string_view name, int& t);
        template <> void DrawType<std::string>(const std::string_view name, std::string& t);
        template <> void DrawType<glm::vec3>(const std::string_view name, glm::vec3& t);
        template <> void DrawType<glm::vec4>(const std::string_view name, glm::vec4& t);
        template <> void DrawType<size_t>(const std::string_view name, size_t& t);
        template <> void DrawType<bool>(const std::string_view name, bool& t);
        template <> void DrawType<char>(const std::string_view name, char& t);
    };
}

#include "ImGuiSystem.inl"
