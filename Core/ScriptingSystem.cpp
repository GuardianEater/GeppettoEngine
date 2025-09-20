/*****************************************************************//**
 * \file   ScriptingSystem.cpp
 * \brief  allows adding scripts to entities
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "pch.hpp"

#include "ScriptingSystem.hpp"
#include <Transform.hpp>
#include <RigidBody.hpp>

namespace Client
{
    ScriptingSystem::ScriptingSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    void ScriptingSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Script>>(this, &ScriptingSystem::OnScriptAdded);
        mManager.SubscribeToEvent<Gep::Event::EntityCreated>(this, &ScriptingSystem::OnEntityCreated);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Script>>(this, &ScriptingSystem::OnScriptEditorRender);

        mManager.GetResource<ScriptingResource>().LocateScripts();
    }

    void ScriptingSystem::Update(float dt)
    {
        mManager.ForEachArchetype<Script>([&](Gep::Entity entity, Script& script)
        {
        });
    }

    void ScriptingSystem::OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event)
    {
        Script& script = mManager.GetComponent<Client::Script>(event.entity);
        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();
        
        script.LoadScript(script.path);
    }

    void ScriptingSystem::OnScriptEditorRender(const Gep::Event::ComponentEditorRender<Script>& event)
    {
        Script& script = event.component;

        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();
        EditorResource& er = mManager.GetResource<EditorResource>();
        const std::set<std::filesystem::path>& knownScripts = sr.GetKnownScripts();

        ImGui::Text("Script Path: %s", script.path.string().c_str());
        if (ImGui::Button("Locate new scripts", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            sr.LocateScripts();
        }

        if (ImGui::Button("Reload script", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            script.LoadScript(script.path);
        }

        // drop down for selecting a script

        bool scriptsOpen = ImGui::BeginCombo("Scripts", script.path.filename().string().c_str());

        er.AssetBrowserDropTarget({ ".py" }, [&](const std::filesystem::path& droppedPath)
            {
                script.LoadScript(droppedPath);
            });

        if (scriptsOpen)
        {
            for (const auto& loadedScript : knownScripts)
            {
                bool isSelected = script.path == loadedScript;
                if (ImGui::Selectable(loadedScript.filename().string().c_str(), isSelected))
                {
                    script.LoadScript(loadedScript);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void ScriptingSystem::OnEntityCreated(const Gep::Event::EntityCreated& event)
    {
    }
}


