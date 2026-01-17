/*****************************************************************//**
 * \file   ScriptingSystem.cpp
 * \brief  allows adding scripts to entities
 *
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#include "pch.hpp"

// this
#include "ScriptingSystem.hpp"

// resources
#include "EditorResource.hpp"
#include "ISystem.hpp"

namespace Client
{
    ScriptingSystem::ScriptingSystem(Gep::EngineManager& em)
        : ISystem(em)
        , mScriptingResource(em.GetResource<ScriptingResource>())
    {
    }

    void ScriptingSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Script>>(this, &ScriptingSystem::OnScriptAdded);
        mManager.SubscribeToEvent<Gep::Event::ComponentRemoved<Script>>(this, &ScriptingSystem::OnScriptRemoved);
        mManager.SubscribeToEvent<Gep::Event::EntityCreated>(this, &ScriptingSystem::OnEntityCreated);
        mManager.SubscribeToEvent<Gep::Event::ComponentEditorRender<Script>>(this, &ScriptingSystem::OnScriptEditorRender);
        mManager.SubscribeToEvent<Gep::Event::EngineStateChanged>(this, &ScriptingSystem::OnEngineStateChanged);
        

        mManager.GetResource<ScriptingResource>().LocateScripts();
    }

    void ScriptingSystem::Update(float dt)
    {
        // on update
        mManager.ForEachArchetype([&](Gep::Entity entity, Script& script)
        {
            mScriptingResource.PyCall(script.update);
        });

        // on late update
        mManager.ForEachArchetype([&](Gep::Entity entity, Script& script)
        {
            mScriptingResource.PyCall(script.late_update);
        });
    }

    void ScriptingSystem::OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event)
    {
        Script& script = mManager.GetComponent<Client::Script>(event.entity);
        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();
        
        py::module module = sr.GetOrLoadModule(script.path);
        sr.BindScriptToModule(script, module);
    }

    void ScriptingSystem::OnScriptRemoved(const Gep::Event::ComponentRemoved<Script>& event)
    {
        if (mManager.IsState(Gep::EngineState::Play))
        {
            // if the entity is enabled disable it first
            if (mManager.IsEnabled(event.entity))
                mScriptingResource.PyCall(event.component.on_disable);

            // call destroy on the entity
            mScriptingResource.PyCall(event.component.on_destroy);
        }
    }

    void ScriptingSystem::OnScriptEditorRender(const Gep::Event::ComponentEditorRender<Script>& event)
    {
        Script& script = *event.components[0];

        EditorResource& er = mManager.GetResource<EditorResource>();
        const std::set<std::filesystem::path>& knownScripts = mScriptingResource.GetKnownScripts();

        ImGui::Text("Script Path: %s", script.path.string().c_str());
        if (ImGui::Button("Locate new scripts", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            mScriptingResource.LocateScripts();
        }

        // drop down for selecting a script

        bool scriptsOpen = ImGui::BeginCombo("Scripts", script.path.filename().string().c_str());

        er.AssetBrowserDropTarget({ ".py" }, [&](const std::filesystem::path& droppedPath)
        {
            script.path = droppedPath;
        });

        if (scriptsOpen)
        {
            for (const auto& loadedScript : knownScripts)
            {
                bool isSelected = script.path == loadedScript;
                if (ImGui::Selectable(loadedScript.filename().string().c_str(), isSelected))
                {
                    script.path = loadedScript;
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

    void ScriptingSystem::OnEngineStateChanged(const Gep::Event::EngineStateChanged& event)
    {
        if (event.newState == Gep::EngineState::Play)
        {
            mScriptingResource.ReloadAllModules();

            mManager.ForEachArchetype([&](Gep::Entity entity, Script& script)
            {
                py::module module = mScriptingResource.GetOrLoadModule(script.path.string());
                mScriptingResource.BindScriptToModule(script, module);
            });
            
            mManager.ForEachArchetype([&](Gep::Entity entity, Script& script) 
            {
                if (mManager.IsEnabled(entity))
                    mScriptingResource.PyCall(script.on_enabled);
            });

            mManager.ForEachArchetype([&](Gep::Entity entity, Script& script)
            {
                mScriptingResource.PyCall(script.on_start);
            });
        }
    }
}


