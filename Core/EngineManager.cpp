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
        mComponentDatas.reserve(MAX_COMPONENTS);
    }

    EngineManager::~EngineManager()
    {
        DestroyAllEntities();

        mSystems.clear();
        mSystemsToUpdate.clear();
        mResources.clear();
    }

    void EngineManager::FrameStart()
    {
        mFrameStartTime = std::chrono::high_resolution_clock::now();

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

        auto endTime = std::chrono::high_resolution_clock::now();
        mDeltaTime = std::chrono::duration<float>(endTime - mFrameStartTime).count();
    }

    bool EngineManager::Running() const
    {
        return mIsRunning;
    }

    float EngineManager::GetDeltaTime() const
    {
        return mDeltaTime;
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

    void EngineManager::DestroyAllEntities()
    {
        for (const auto& [entity, entityData] : mEntityDatas)
        {
            MarkEntityForDestruction(entity);
        }

        DestroyMarkedEntities();
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
        Entity id = mEntityDatas.emplace();
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

        auto ancestors = GetAncestors(parent);
        if (std::find(ancestors.begin(), ancestors.end(), child) != ancestors.end())
        {
            Log::Error("AttachEntity() failed, Parent Entity: [", parent, "] is a child of Child Entity: [", child, "]");
            return;
        }

        // if the new child has a parent currently, remove the child from its parent
        if (HasParent(child))
            DetachEntity(child);

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

    size_t EngineManager::GetChildCount(Entity parent) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("GetChildCount() failed, Entity: [", parent, "] does not exist");
            return 0;
        }

        return mEntityDatas.at(parent).children.size();
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

        return mEntityDatas.at(parent).children.size();
    }

    std::vector<Entity> EngineManager::GetRootEntities() const
    {
        std::vector<Entity> rootEntities;

        const auto& entities = GetEntities();

        for (Entity entity : entities)
        {
            if (!HasParent(entity))
            {
                rootEntities.push_back(entity);
            }
        }

        return rootEntities;
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

    nlohmann::json EngineManager::SaveEntity(Entity entity) const
    {
        nlohmann::json entityJson = nlohmann::json::object();
        nlohmann::json componentsJson = nlohmann::json::array();
        nlohmann::json childrenJson = nlohmann::json::array();

        ForEachComponent(entity, [&](const Gep::ComponentData& componentData)
        {
            nlohmann::json componentJson = componentData.save(entity);
            componentsJson.push_back(componentJson);
        });

        ForEachChild(entity, [&](Gep::Entity child)
        {
            nlohmann::json childJson = SaveEntity(child);
            childrenJson.push_back(childJson);
        });

        entityJson["children"] = childrenJson;
        entityJson["components"] = componentsJson;
        entityJson["id"] = entity;

        return entityJson;
    }

    Entity EngineManager::LoadEntity(const nlohmann::json& entityJson)
    {
        Gep::Entity entity = CreateEntity();

        if (!entityJson.contains("components"))
        {
            Gep::Log::Error("Entity json does not contain components");
        }
        else
        {
            const nlohmann::json& componentsJson = entityJson["components"];
            for (const nlohmann::json& componentJson : componentsJson)
            {
                const std::string componentName = componentJson["type"].get<std::string>();
                const nlohmann::json& componentDataJson = componentJson["data"];

                size_t componentIndex = mComponentNameToIndex.at(componentName);
                ComponentData& componentData = mComponentDatas.at(componentIndex);

                componentData.load(entity, componentDataJson);
            }
        }

        if (!entityJson.contains("children"))
        {
            Gep::Log::Error("Entity json does not contain children");
        }
        else
        {
            const nlohmann::json& childrenJson = entityJson["children"];
            for (const nlohmann::json& childJson : childrenJson)
            {
                Gep::Entity child = LoadEntity(childJson);
                AttachEntity(entity, child);
            }
        }

        // will be used later to map old ids to new ids, in the case where a component references another entity
        if (!entityJson.contains("id"))
        {
            Gep::Log::Error("Entity json does not contain an id");
        }

        return entity;
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

        SubscribeToEvent<Event::EntityDestroyed>(this, &EngineManager::OnEntityDestroyed);
    }


    void EngineManager::Update()
    {
        for (const auto& system : mSystemsToUpdate)
        {
            system->Update(mDeltaTime);
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

    void EngineManager::OnEntityDestroyed(const Event::EntityDestroyed& event)
    {
        Log::Important("Entity: [", event.entity, "] has been destroyed");
    }
}
