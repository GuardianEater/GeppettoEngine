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

#include <rfl.hpp>

#include <glm\glm.hpp>

#include "Logger.hpp"
#include "TypeID.hpp"
#include "gtl/type_list.hpp"
#include "EngineManager.hpp"

// client

namespace Gep
{
    class OpenGLRenderer;
}

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
        bool DrawEntityNode(Gep::Entity entity, const std::string& displayName, bool selected, const ImVec4& defaultColor);
        void DrawQuickTest();
        void DrawGBufferTextures();

        std::vector<Gep::Entity> SearchEntities(const std::vector<Gep::Entity>& entities, const std::string& searchTerm);
        void SetAssetBrowserPath(const std::filesystem::path& newPath);
        void ReloadAssetBrowser();

    private:
        EditorResource& mEditorResource;
        Gep::OpenGLRenderer& mRenderer;
        std::filesystem::path mAssetBrowserPath;
        std::vector<std::filesystem::directory_entry> mAssetBrowserEntries; // the files that are visible from the asset browser path
        std::vector<std::function<void(std::span<Gep::Entity>)>> mComponentInspectorPanels; // component index -> function to draw the inspector

    public:

        ImGuiSystem(Gep::EngineManager& em);

        // special events //
        template <typename... ComponentTypes>
        void OnComponentsRegistered(gtl::type_list<ComponentTypes...> componentTypes);

        // events //
        void OnEntityCreated(const Gep::Event::EntityCreated& event);
        void OnEntityDestroyed(const Gep::Event::EntityDestroyed& event);
        void OnMouseScrolled(const Gep::Event::MouseScrolled& event);
        void OnFileDropped(const Gep::Event::FileDropped& event);
        void OnEntityAttached(const Gep::Event::EntityAttached& event);
        void OnEntityDetached(const Gep::Event::EntityDetached& event);

        // game loop //
        void Initialize() override;
        void Update(float dt) override;

        // helpers //
        void DrawEntities(const std::vector<Gep::Entity>& entities, float dt);
        void DrawAssetBrowser();
        void DrawToolbar();
        void ShowControlBar();
    };
}

#include "ImGuiSystem.inl"
