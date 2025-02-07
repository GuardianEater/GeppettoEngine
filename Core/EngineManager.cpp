/*****************************************************************//**
 * \file   EngineManager.cpp
 * \brief  implementation of the EngineManager
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "EngineManager.hpp"

#include "KeyedVector.hpp"

#include "Logger.hpp"

namespace Gep
{
    EngineManager::EngineManager()
        : mMarkedEntities()
        , mEntityDatas()
        , mMarkedComponents()
        , mComponentDatas()
        , mNextComponentBitPos(0)
        , mIsRunning(true)
        , mEntityGroups()
    {
        SubscribeToEvent<Event::WindowClosing>(this, &EngineManager::OnWindowClosing);
        // needed so the unerlying vector does not reallocate.
        mComponentDatas.reserve(MAX_COMPONETS);
    }

    void EngineManager::FrameStart()
    {
        // loop though all of the systems in order and call their start functions
        for (auto& system : mSystemsToUpdate)
        {
            system->FrameStart();
        }
    }

    void EngineManager::FrameEnd()
    {
        // loop though all of the systems in referse order and call their exit functions
        for (auto systemIt = mSystemsToUpdate.rbegin(); systemIt != mSystemsToUpdate.rend(); ++systemIt)
        {
            (*systemIt)->FrameEnd();
        }
    }

    bool EngineManager::Running() const
    {
        return mIsRunning;
    }

    void EngineManager::SetSignature(Entity entity, Signature signature)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetSignature() failed, Entity: [", entity, "] does not exist");
            return;
        }

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
            Log::Error("GetSignature() failed, Entity: [", entity, "] does not exist");
            return Signature();
        }
        // Put this entity's signature into the array
        return mEntityDatas.at(entity).signature;
    }

    void EngineManager::MarkEntityForDestruction(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("MarkEntityForDestruction() failed, Entity: [", entity, "] does not exist");
            return;
        }

        SignalEvent(Event::EntityDestroyed{ .entity = entity });// calls subscriber functions 

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
    void EngineManager::DestroyEntity(Entity entity)
    {
        ForEachComponent(entity, [&](const ComponentData& componentData)
        {
            DestroyComponent(componentData.index, entity);
        });

        // removes the entity from any systems it might have been in
        for (auto& [groupSignature, entities] : mEntityGroups)
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());

        // for each child of the entity, remove the parent
        if (HasChild(entity))
            for (Entity child : GetChildren(entity))
                DetachEntity(child);
        
        if (HasParent(entity))
            DetachEntity(entity);

        mEntityDatas.erase(entity);

        Log::Trace("Destroyed Entity: [", entity, "]");
    }

    Entity EngineManager::CreateEntity()
    {
        Entity id = mEntityDatas.insert({});
        SetSignature(id, 0);

        Log::Trace("Created Entity: [", id, "]");

        return id;
    }

    Entity EngineManager::DuplicateEntity(Entity entity)
    {
        // get the components off of the entity
        Entity newEntity = CreateEntity();

        ForEachComponent(entity, [&](const ComponentData& componentData)
        {
            componentData.copy(newEntity, entity);
        });

        std::vector<Entity> children = GetChildren(entity);
        for (Entity child : children)
        {
            Entity newChild = DuplicateEntity(child);
            AttachEntity(newEntity, newChild);
        }

        return newEntity;
    }

    void EngineManager::AttachEntity(Entity parent, Entity child)
    {
        if (parent == child)
        {
            Log::Error("AttachEntity() failed, Entity: [", parent, "] cannot be attached to itself");
            return;
        }

        if (!EntityExists(parent))
        {
            Log::Error("AttachEntity() failed, Parent Entity: [", parent, "] does not exist");
            return;
        }

        if (!EntityExists(child))
        {
            Log::Error("AttachEntity() failed, Child Entity: [", child, "] does not exist");
            return;
        }

        if (mEntityDatas[child].parent == parent)
        {
            Log::Error("AttachEntity() failed, Child Entity: [", child, "] is already attached to Parent Entity: [", parent, "]");
            return;
        }

        // if the new child has a parent currently, remove the child from its parent
        if (HasParent(child))
        {
            DetachEntity(child);
        }

        mEntityDatas.at(parent).children.push_back(child);
        mEntityDatas.at(child).parent = parent;
    }

    void EngineManager::DetachEntity(Entity child)
    {
        if (!EntityExists(child))
        {
            Log::Error("DetachEntity() failed, Entity: [", child, "] does not exist");
            return;
        }

        if (!HasParent(child))
        {
            Log::Error("DetachEntity() failed, Entity: [", child, "] does not have a parent");
            return;
        }

        Entity parent = mEntityDatas[child].parent;

        std::vector<Entity>& children = mEntityDatas[parent].children;
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
        mEntityDatas.at(child).parent = INVALID_ENTITY;
    }

    bool EngineManager::HasParent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("HasParent() failed, Entity: [", entity,"] does not exist");
            return false;
        }

        if (mEntityDatas.at(entity).parent == INVALID_ENTITY)
        {
            return false;
        }

        return true;
    }

    Entity EngineManager::GetParent(Entity child) const
    {
        if (!EntityExists(child))
        {
            Log::Error("GetParent() failed, Entity: [", child, "] does not exist");
            return INVALID_ENTITY;
        }

        return mEntityDatas.at(child).parent;
    }

    std::vector<Entity> EngineManager::GetAncestors(Entity entity) const
    {
        std::vector<Entity> ancestors{};

        if (!EntityExists(entity))
        {
            Log::Error("GetAncestors() failed, Entity: [", entity, "] does not exist");
            return ancestors;
        }

        Entity parent = GetParent(entity);

        while (parent != INVALID_ENTITY)
        {
            ancestors.push_back(parent);
            parent = GetParent(parent);
        }

        return ancestors;
    }

    Entity EngineManager::GetRoot(Entity child) const
    {
        if (!EntityExists(child))
        {
            Log::Error("GetRoot() failed, Entity: [", child, "] does not exist");
            return INVALID_ENTITY;
        }

        Entity parent = GetParent(child);
        while (parent != INVALID_ENTITY)
        {
            child = parent;
            parent = GetParent(child);
        }

        return child;
    }

    std::vector<Entity> EngineManager::GetSiblings(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("GetSiblings() failed, Entity: [", entity, "] does not exist");
            return std::vector<Entity>();
        }
        Entity parent = GetParent(entity);
        if (parent == INVALID_ENTITY)
        {
            Log::Error("GetSiblings() failed, Entity: [", entity, "] does not have a parent");
            return std::vector<Entity>();
        }

        return GetChildren(parent);
    }

    std::vector<Entity> EngineManager::GetChildren(Entity parent) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("GetChildren() failed, Entity: [", parent, "] does not exist");
            return std::vector<Entity>();
        }

        return mEntityDatas.at(parent).children;
    }

    bool EngineManager::HasChild(Entity parent) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("HasChild() failed, Entity: [", parent, "] does not exist");
            return false;
        }

        if (mEntityDatas.at(parent).children.size() == 0)
        {
            return false;
        }

        return true;
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
            Log::Error("GetComponentSignatures() failed, Entity: [", entity, "] does not exist");
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

    const Gep::keyed_vector<ComponentData>& EngineManager::GetComponentDatas() const
    {
        return mComponentDatas;
    }

    void EngineManager::DestroyMarkedComponents()
    {
        for (const auto& [componentID, entity] : mMarkedComponents)
        {
            DestroyComponent(componentID, entity);
        }

        mMarkedComponents.clear();
    }

    void EngineManager::DestroyComponent(uint64_t componentID, Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("DestroyComponent() failed, Entity: [", entity, "] does not exist");
            return;
        }

        //GetComponentArray(componentID)->erase(entity);
        GetComponentArray(componentID)->erase(entity);

        Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
        Signature componentSignature = mComponentDatas.at(componentID).signature; // gets the signature of the component

        entitySignature &= ~componentSignature; // removes the component from the entity signature

        SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly removed component
    }

    bool EngineManager::HasComponent(uint64_t componentType, Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("HasComponent() failed, Entity: [", entity, "] does not exist");
            return false;
        }

        Signature componentSig = mComponentDatas.at(componentType).signature;
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

    void EngineManager::ResolveEvents()
    {
        //for each type of event
        while (!mEventQueue.empty())
        {
            const auto& [id, eventData] = mEventQueue.front();

            //get the subscribers for this event type
            const auto& subscribers = mEventDatas[id].subscribers;
            for (auto& subscriber : subscribers)
            {
                subscriber(eventData);
            }

            mEventQueue.pop_front();
        }
    }

    std::shared_ptr<IComponentArray> EngineManager::GetComponentArray(uint64_t componentID)
    {
        return mComponentDatas.at(componentID).array;
    }

    const std::shared_ptr<IComponentArray> EngineManager::GetComponentArray(uint64_t componentID) const
    {
        return mComponentDatas.at(componentID).array;
    }

    void EngineManager::OnWindowClosing(const Event::WindowClosing& event)
    {
        mIsRunning = false;
    }
}
