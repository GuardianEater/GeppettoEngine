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
#include <sol/sol.hpp>

namespace Client
{
    ScriptingSystem::ScriptingSystem(Gep::EngineManager& em)
        : ISystem(em)
    {
    }

    void ScriptingSystem::Initialize()
    {
        mManager.SubscribeToEvent<Gep::Event::ComponentAdded<Script>>(this, &ScriptingSystem::OnScriptAdded);

        mManager.GetResource<ScriptingResource>().LocateScripts();
    }

    void ScriptingSystem::Update(float dt)
    {
        sol::state& lua = mManager.GetResource<ScriptingResource>().GetLua();

        const std::vector<Gep::Entity>& entities = mManager.GetEntities<Script>();
        for (Gep::Entity entity : entities)
        {
            Script& script = mManager.GetComponent<Client::Script>(entity);
            sol::table self = lua.create_table();

            mManager.ForEachComponent(entity, [&](const Gep::ComponentData& data)
            {
                mSetComponentMemberReferences[data.index](entity, self);
            });

            if (!script.env.valid())
            {
                // this will always happen once because component construction is not yet deffered
                Gep::Log::Error("ScriptingSystem::Update() failed, script environment is invalid on entity: [", entity, "]");

                continue;
            }

            script.env["self"] = self;

            if (script.update.valid())
            {
                sol::protected_function_result updateResult = script.update(dt);

                if (!updateResult.valid())
                {
                    sol::error err = updateResult;
                    Gep::Log::Error("Error running script: ", err.what());
                    script.update = sol::nil; // prevents the crashed script from running further
                    script.exit = sol::nil;
                }
            }
        }

    }

    void ScriptingSystem::OnScriptAdded(const Gep::Event::ComponentAdded<Script>& event)
    {
        Script& script = mManager.GetComponent<Client::Script>(event.entity);
        ScriptingResource& sr = mManager.GetResource<ScriptingResource>();
        
        script.LoadScript(sr.GetLua(), script.path);
    }
}


