/*****************************************************************//**
 * \file   EngineManager.cpp
 * \brief  implementation of the EngineManager
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#include "EngineManager.hpp"

namespace Gep
{
    EngineManager::EngineManager()
        : mAvailableEntities()
        , mMarkedEntities()
        , mEntitySignatures()
        , mComponentIDs()
        , mMarkedComponents()
        , mComponentArrays()
        , mNextComponentID(0)
        , mIsRunning(true)
    {
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
        {
            mAvailableEntities.push_back(entity);
        }

        mApplication.SetKeyCallback(*this, &EngineManager::SignalEvent<Event::KeyPressed>);
    }

    void EngineManager::Start()
    {
        mApplication.Initialize_GLFW();
        mApplication.Initialize_ImGui();
    }

    void EngineManager::End()
    {
        mApplication.End_ImGui();
        mApplication.End_GLFW();
    }

    void EngineManager::FrameStart()
    {
        mApplication.FrameStart_GLFW();
        mApplication.FrameStart_ImGui();
    }

    void EngineManager::FrameEnd()
    {
        mApplication.FrameEnd_ImGui();
        mApplication.FrameEnd_GLFW();
    }

    bool EngineManager::Running() const
    {
        return mApplication.Running();
    }

    void EngineManager::SetSignature(Entity entity, Signature signature)
    {
        // Put this entity's signature into the array
        mEntitySignatures[entity] = signature;

        for (auto& [groupSignature, entities] : mEntityGroups)
        {
            // checks if the entities signature is the same as the groups signature, if so add the entity to the group
            if ((signature & groupSignature) == groupSignature)
            {
                entities.insert(entity);
            }
            else
            {
                entities.erase(entity);
            }
        }
    }

    Signature EngineManager::GetSignature(Entity entity) const
    {
        // Put this entity's signature into the array
        return mEntitySignatures.at(entity);
    }

    void EngineManager::MarkEntityForDestruction(Entity entity)
    {
        SignalEvent<Event::EntityDestroyed>({ entity });// calls subscriber functions 

        mMarkedEntities.push_back(entity);
    }

    void EngineManager::DestroyMarkedEntities()
    {
        for (const Entity entity : mMarkedEntities)
        {
            DestroyEntity(entity);
        }

        mMarkedEntities.clear();
    }

    // adds the id back to the id pool
    void EngineManager::DestroyEntity(const Entity entity)
    {
        // destroys each component on an entity if it has one
        for (const auto& [componentID, componentArray] : mComponentArrays)
        {
            if (HasComponent(entity, componentID))
                DestroyComponent(entity, componentID);
        }

        // removes the entity from any systems it might have been in
        for (auto& [groupSignature, entities] : mEntityGroups)
            entities.erase(entity);

        mEntitySignatures[entity].reset();

        mAvailableEntities.push_back(entity);
    }

    Entity EngineManager::CreateEntity()
    {
        Entity id = mAvailableEntities.back();
        mAvailableEntities.pop_back();

        SetSignature(id, 0);

        return id;
    }

    void EngineManager::DestroyMarkedComponents()
    {
        for (const auto& [entity, componentID] : mMarkedComponents)
        {
            DestroyComponent(entity, componentID);
        }

        mMarkedComponents.clear();
    }

    void EngineManager::DestroyComponent(Entity entity, uint64_t component)
    {
        GetComponentArray(component)->Erase(entity);

        Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
        entitySignature.set(mComponentIDs.at(component), false); // removes the components id from the entities signature

        SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly removed component
    }

    bool EngineManager::HasComponent(const Entity entity, const uint64_t componentID) const
    {
        Signature componentSig;
        componentSig.set(mComponentIDs.at(componentID));
        return ((GetSignature(entity) & componentSig) == componentSig);
    }

    void EngineManager::Initialize()
    {
        for (const auto& system : mSystemsToUpdate)
        {
            system->Initialize();
        }
    }


    void EngineManager::Update(float dt)
    {
        for (const auto& system : mSystemsToUpdate)
        {
            system->Update(dt);
        }
    }

    void EngineManager::Exit()
    {
        // loop though all of the systems in referse order and call their exit functions
        for (auto systemIt = mSystemsToUpdate.rbegin(); systemIt != mSystemsToUpdate.rend(); ++systemIt)
        {
            (*systemIt)->Exit();
        }
    }

    std::shared_ptr<IComponentArray> EngineManager::GetComponentArray(uint64_t typeID)
    {
        return mComponentArrays.at(typeID);
    }
}
