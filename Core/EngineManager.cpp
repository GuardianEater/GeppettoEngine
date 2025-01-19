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
        , mEntityDatas()
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
        mEntityDatas[entity].signature = signature;

        for (auto& [groupSignature, entities] : mEntityGroups)
        {
            // checks if the entities signature is the same as the groups signature, if so add the entity to the group
            if ((signature & groupSignature) == groupSignature)
            {
                // if the entity is not in the group, add it
                if (std::find(entities.begin(), entities.end(), entity) == entities.end())
                    entities.push_back(entity);
            }
            else
            {
                // if the entity is in the group, remove it
                entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
            }
        }
    }

    Signature EngineManager::GetSignature(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("Entity does not exist");
            return Signature();
        }
        // Put this entity's signature into the array
        return mEntityDatas.at(entity).signature;
    }

    void EngineManager::MarkEntityForDestruction(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("Entity does not exist");
            return;
        }

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
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());

        mEntityDatas[entity].signature.reset();

        // for each child of the entity, remove the parent
        for (Entity child : GetChildren(entity))
        {
            DetachEntity(child);
        }

        mAvailableEntities.push_back(entity);

        Log::Trace("Destroyed Entity: [", entity, "]");
    }

    Entity EngineManager::CreateEntity()
    {
        Entity id = mAvailableEntities.back();
        mAvailableEntities.pop_back();
        SetSignature(id, 0);

        Log::Trace("Created Entity: [", id, "]");

        return id;
    }

    void EngineManager::AttachEntity(Entity parent, Entity child)
    {
        if (!EntityExists(parent))
        {
            Log::Error("Parent entity does not exist");
            return;
        }

        if (!EntityExists(child))
        {
            Log::Error("Child entity does not exist");
            return;
        }

        if (mEntityDatas[child].parent == parent)
        {
            Log::Error("Child is already attached to this parent");
            return;
        }

        // if the new child has a parent currently, remove the child from its parent
        Entity otherParent = mEntityDatas[child].parent;
        if (EntityExists(otherParent))
        {
            DetachEntity(child);
        }

        mEntityDatas.at(parent).children.push_back(child);
        mEntityDatas.at(child).parent = parent;
    }

    // 
    void EngineManager::DetachEntity(Entity child)
    {
        if (!EntityExists(child))
        {
            Log::Error("Entity does not exist");
            return;
        }

        if (!HasParent(child))
        {
            Log::Error("Entity does not have a parent");
            return;
        }

        Entity parent = mEntityDatas[child].parent;
        if (!EntityExists(parent))
        {
            Log::Error("Parent entity does not exist");
            return;
        }

        std::vector<Entity>& children = mEntityDatas[parent].children;
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
        mEntityDatas.at(child).parent = INVALID_ENTITY;
    }

    bool EngineManager::HasParent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("Entity does not exist");
            return false;
        }

        return (mEntityDatas.at(entity).parent != INVALID_ENTITY);
    }

    std::vector<Entity> EngineManager::GetChildren(Entity parent)
    {
        if (!EntityExists(parent))
        {
            Log::Error("Entity does not exist");
            return std::vector<Entity>();
        }

        return mEntityDatas.at(parent).children;
    }

    bool EngineManager::EntityExists(Entity entity) const
    {
        if (entity == INVALID_ENTITY)
        {
            return false;
        }

        if (!mEntityDatas.contains(entity))
        {
            return false;
        }

        return true;
    }

    std::vector<Signature> EngineManager::GetComponentSignatures(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("Entity does not exist");
            return std::vector<Signature>();
        }

        Signature entitySignature = GetSignature(entity);

        std::vector<Signature> componentSignatures;

        for (int i = 0; i < entitySignature.size(); ++i)
        {
            if (entitySignature[i])
            {
                Signature componentSig; componentSig.set(i);
                componentSignatures.push_back(componentSig);
            }
        }

        return componentSignatures;
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
        if (!EntityExists(entity))
        {
            Log::Error("Entity does not exist");
            return;
        }

        GetComponentArray(component)->Erase(entity);

        Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
        entitySignature.set(mComponentIDs.at(component), false); // removes the components id from the entities signature

        SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly removed component
    }

    bool EngineManager::HasComponent(const Entity entity, const uint64_t componentID) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("Entity does not exist");
            return false;
        }

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
