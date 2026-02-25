/*****************************************************************//**
 * \file   EngineManager.cpp
 * \brief  implementation of the EngineManager
 * 
 * \author Travis Gronvold
 * \date   January 2025
 *********************************************************************/

#include "pch.hpp"

#include "EngineManager.hpp"

#include "gtl/keyed_vector.hpp"

#include "Logger.hpp"

namespace Gep
{
    EngineManager::EngineManager()
    {
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
        for (const auto index : mSystemsToUpdate)
        {
            SystemData& sd = mSystems.at(index);

            // start system timer
            auto systemStartTime = std::chrono::high_resolution_clock::now();

            // if the system matches the policy run the system
            if (sd.executionPolicy >= mState)
                sd.system->FrameStart();

            // computer time spent in frame start
            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInFrameStart = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }

    void EngineManager::FrameEnd()
    {
        // loop though all of the systems in referse order and call their exit functions
        for (auto indexIt = mSystemsToUpdate.rbegin(); indexIt != mSystemsToUpdate.rend(); ++indexIt)
        {
            SystemData& sd = mSystems.at(*indexIt);

            // start system timer
            auto systemStartTime = std::chrono::high_resolution_clock::now();

            // if the system matches the policy run the system
            if (sd.executionPolicy >= mState)
                sd.system->FrameEnd();

            // computer time spent in frame end
            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInFrameEnd = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        mDeltaTime = std::chrono::duration<float>(endTime - mFrameStartTime).count() - mExcludedDeltaTime;
        mExcludedDeltaTime = 0.0f;
    }

    bool EngineManager::IsRunning() const
    {
        return mIsRunning;
    }

    float EngineManager::GetDeltaTime() const
    {
        return mDeltaTime;
    }

    void EngineManager::Shutdown()
    {
        mIsRunning = false;
    }

    void EngineManager::SetState(EngineState state)
    {
        SignalEvent(Gep::Event::EngineStateChanged{ mState, state });
        mState = state;
    }

    bool EngineManager::IsState(EngineState state) const
    {
        return mState == state;
    }

    EngineState EngineManager::GetCurrentState() const
    {
        return mState;
    }

    void EngineManager::SetSignature(Entity entity, Signature signature)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetSignature() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas[entity].signature = signature;
    }

    Signature EngineManager::GetSignature(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("GetSignature() failed, Entity: [", entity, "] does not exist");
            return Signature{};
        }

        return mEntityDatas.at(entity).signature;
    }

    void EngineManager::SetName(Entity entity, const std::string& name)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetName() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas.at(entity).name = name;
    }

    const std::string& EngineManager::GetName(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("GetName() failed, Entity: [", entity, "] does not exist");
        }

        return mEntityDatas.at(entity).name;
    }

    void EngineManager::SetUUID(Entity entity, const gtl::uuid& uuid)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetUUID() failed, Entity: [", entity, "] does not exist");
            return;
        }

        // if the entity already had a uuid set, erase the old one
        const gtl::uuid& oldid = GetUUID(entity);
        if (oldid.is_valid() && mUUIDToEntity.contains(oldid))
        {
            mUUIDToEntity.erase(oldid);
        }

        mUUIDToEntity[uuid] = entity;
        mEntityDatas.at(entity).uuid = uuid;
    }

    const gtl::uuid& EngineManager::GetUUID(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("GetUUID() failed, Entity: [", entity, "] does not exist");
        }

        return mEntityDatas.at(entity).uuid;
    }

    void EngineManager::MarkEntityForDestruction(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("MarkEntityForDestruction() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mMarkedEntities.push_back(entity);
    }

    void EngineManager::DestroyMarkedEntities()
    {
        for (const Entity entity : mMarkedEntities)
        {
            // the destroy of one entity could potentially destroy other entities so
            // must check if the entitiy exists before destruction
            if (EntityExists(entity))
                DestroyEntity(entity);
        }

        mMarkedEntities.clear();
    }

    void EngineManager::DestroyAllEntities()
    {
        Gep::Log::Trace("Destroying all entities...");
        ExcludeFromDt([&]
        {
            for (const auto& [entity, entityData] : mEntityDatas)
            {
                MarkEntityForDestruction(entity);
            }

            DestroyMarkedEntities();
        });

        Gep::Log::Info("All entities destroyed");
    }

    // adds the entity back to the entity pool
    void EngineManager::DestroyEntity(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Gep::Log::Error("DestroyEntity() failed, entity: [", entity, "] does not exist");
            return;
        }

        // because of ordering all events are signaled top down, and in those events the hierarchy is 100% valid
        SignalEvent(Event::EntityDestroyed{ entity });

        // signal the event that these components are being destroyed
        ForEachComponent(entity, [&](const ComponentData& componentData)
        {
            componentData.onRemove(entity);
        });

        // note: this has to be a vector by value because detach changes the underlying storage
        std::vector<Entity> children = GetChildren(entity);
        for (Entity child : children)
        {
            DestroyEntity(child);
        }

        Archetype_Clear(entity);

        // on detach event: the entity may have had all of its components and children removed, should probably seperate the signal and the 'do'
        if (HasParent(entity))
            DetachEntity(entity);

        mUUIDToEntity.erase(mEntityDatas.at(entity).uuid); // makesure the uuid is unbound to this entity when destroyed
        mEntityDatas.erase(entity);

        Log::Trace("Destroyed Entity: [", entity, "]");
    }

    Entity EngineManager::CreateEntity(const std::string& name, const gtl::uuid& uuid)
    {
        Entity entity = mEntityDatas.emplace();

        SetName(entity, name);

        if (uuid.is_valid()) // allows construction of an entity without a predefined uuid, will just generate one instead
            SetUUID(entity, uuid);
        else
            SetUUID(entity, gtl::generate_uuid());

        SignalEvent(Event::EntityCreated{ entity });

        Log::Trace("Created Entity: [", entity, "]");

        return entity;
    }

    Entity EngineManager::DuplicateEntity(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Gep::Log::Error("DuplicateEntity() failed, entity: [", entity, "] does not exist");
            return INVALID_ENTITY;
        }

        nlohmann::json entityData = SaveEntity(entity);
        Entity newEntity = LoadEntity(entityData, false);
        SetUUID(newEntity, gtl::generate_uuid());

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

        debug_if(true) // very expensive validation check but only in debug mode
        {
            auto ancestors = GetAncestors(parent); 
            if (std::find(ancestors.begin(), ancestors.end(), child) != ancestors.end())
            {
                Log::Error("AttachEntity() failed, Parent Entity: [", parent, "] is a child of Child Entity: [", child, "]");
                return;
            }
        }

        // if the new child has a parent currently, remove the child from its parent
        if (HasParent(child))
            DetachEntity(child);

        mEntityDatas.at(parent).children.push_back(child);
        mEntityDatas.at(child).parent = parent;

        SignalEvent(Event::EntityAttached{ child, parent });
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

        SignalEvent(Event::EntityDetached({ child, parent }));

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

        if (!EntityExists(mEntityDatas.at(entity).parent)) // note this is not an error
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

        while (EntityExists(parent))
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
        std::vector<Entity> ancestors = GetAncestors(child);

        if (ancestors.empty())
            return child;

        return ancestors.back();
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

    const std::vector<Entity>& EngineManager::GetChildren(Entity parent) const
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

        return !mEntityDatas.at(parent).children.empty();
    }

    std::vector<Entity> EngineManager::GetRoots()
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

    size_t EngineManager::GetEntityCount() const
    {
        return mEntityDatas.size();
    }

    bool EngineManager::IsEnabled(Gep::Entity entity) const
    {
        return mEntityDatas.at(entity).active;
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

    Entity EngineManager::FindEntity(const gtl::uuid& uuid) const
    {
        if (!uuid.is_valid()) 
            return INVALID_ENTITY;

        auto it = mUUIDToEntity.find(uuid);
        if (it == mUUIDToEntity.end())
        {
            return INVALID_ENTITY;
        }

        return it->second;
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
        entityJson["uuid"] = GetUUID(entity).to_string();
        entityJson["name"] = GetName(entity);

        return entityJson;
    }

    Entity EngineManager::LoadEntity(const nlohmann::json& entityJson, bool readUUID)
    {
        Gep::Entity entity = CreateEntity();

        if (!entityJson.contains("components"))
        {
            Gep::Log::Warning("Entity json does not contain components");
        }
        else
        {
            const nlohmann::json& componentsJson = entityJson["components"];
            for (const nlohmann::json& componentJson : componentsJson)
            {
                const std::string componentName = componentJson["type"].get<std::string>();
                const nlohmann::json& componentDataJson = componentJson["data"];

                if (!mComponentNameToIndex.contains(componentName))
                {
                    Gep::Log::Warning("The given entity json had an unrecognized component data in it of name ",componentName,", it will be skipped");
                    continue;
                }

                size_t componentIndex = mComponentNameToIndex.at(componentName);
                ComponentData& componentData = mComponentDatas.at(componentIndex);

                componentData.load(entity, componentDataJson);
            }
        }

        if (!entityJson.contains("children"))
        {
            Gep::Log::Warning("Entity json does not contain children");
        }
        else
        {
            const nlohmann::json& childrenJson = entityJson["children"];
            for (const nlohmann::json& childJson : childrenJson)
            {
                Gep::Entity child = LoadEntity(childJson, readUUID);
                AttachEntity(entity, child);
            }
        }

        // will be used later to map old ids to new ids, in the case where a component references another entity
        if (!entityJson.contains("uuid"))
        {
            Gep::Log::Warning("Entity json does not contain a uuid");
        }
        else
        {
            if (readUUID) // CreateEntity generates a uuid by default
            {
                gtl::uuid uuid = gtl::to_uuid(entityJson["uuid"].get<std::string>());
                if (uuid.is_valid())
                    SetUUID(entity, uuid);
            }
        }
        
        if (!entityJson.contains("name"))
        {
            Gep::Log::Warning("Entity json does not contain a name");
        }
        else
        {
            SetName(entity, entityJson["name"].get<std::string>());
        }

        return entity;
    }

    const gtl::keyed_vector<ComponentData>& EngineManager::GetComponentDatas() const
    {
        return mComponentDatas;
    }

    void EngineManager::DestroyMarkedComponents()
    {
        for (const auto& [componentIndex, entity] : mMarkedComponents)
        {
            ComponentData& data = mComponentDatas.at(componentIndex);
            data.remove(entity);
        }

        mMarkedComponents.clear();
    }

    bool EngineManager::HasComponent(uint64_t componentIndex, Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("HasComponent() failed, Entity: [", entity, "] does not exist");
            return false;
        }
        if (!ComponentIsRegistered(componentIndex))
        {
            Log::Error("HasComponent() failed, component: [", entity, "] is not registered");
            return false;
        }

        Signature signature = GetSignature(entity);
        return signature.test(componentIndex);
    }

    bool EngineManager::ComponentIsRegistered(uint64_t componentIndex) const
    {
        return mComponentDatas.contains(componentIndex);
    }

    const std::unordered_map<Signature, Archetype>& EngineManager::GetArchetypes() const
    {
        return mArchetypes;
    }

    const gtl::keyed_vector<SystemData>& EngineManager::GetSystemDatas() const
    {
        return mSystems;
    }

    void EngineManager::Initialize()
    {
        for (const auto& index : mSystemsToUpdate)
        {
            SystemData& sd = mSystems.at(index);

            // start system timer
            auto systemStartTime = std::chrono::high_resolution_clock::now();

            sd.system->Initialize();

            // compute time spent in init
            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInInitialize = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }


    void EngineManager::Update()
    {
        for (const auto& index : mSystemsToUpdate)
        {
            SystemData& sd = mSystems.at(index);

            // start system timer
            auto systemStartTime = std::chrono::high_resolution_clock::now();
            
            // if the system matches the policy run the system
            if (sd.executionPolicy >= mState)
                sd.system->Update(mDeltaTime);

            // compute time spent in update
            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInUpdate = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }

    void EngineManager::Exit()
    {
        // loop though all of the systems in reverse order and call their exit functions
        for (auto systemIt = mSystemsToUpdate.rbegin(); systemIt != mSystemsToUpdate.rend(); ++systemIt)
        {
            SystemData& sd = mSystems.at(*systemIt);

            // start system timer
            auto systemStartTime = std::chrono::high_resolution_clock::now();

            sd.system->Exit();

            // compute time spent in exit
            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInExit = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }

    const SystemData& EngineManager::GetSystemData(uint64_t systemIndex) const
    {
        return mSystems.at(systemIndex);
    }

    void EngineManager::Archetype_Create(Signature signature)
    {
        if (mArchetypes.contains(signature))
        {
            Log::Critical("Cannot create archetype the given signature already exists: [", signature.to_string(), "]");
        }

        Archetype& chunk = mArchetypes[signature];

        chunk.signature = signature;
        chunk.stride += sizeof(Entity); // size of the entity

        ForEachComponentBit(signature, [&](const ComponentData& data)
        {
            chunk.componentOffsets.at(data.index) = chunk.stride;
            chunk.stride += data.size; // size of each component
        });
    }

    void EngineManager::Archetype_Move(Archetype& oldArchetype, Archetype& targetArchetype, glm::u64vec2 oldIndex, glm::u64vec2 targetIndex) const
    {
        if (!oldArchetype.InBounds(oldIndex.x, oldIndex.y))
        {
            Gep::Log::Critical("Archetype_Move() failed, the oldIndex: [", oldIndex, "] was outside of the capacity of the archetype, capacity: [", oldArchetype.EntityCount(), "]");
        }
        if (!targetArchetype.InBounds(targetIndex.x, targetIndex.y))
        {
            Gep::Log::Critical("Archetype_Move() failed, the oldIndex: [", targetIndex, "] was outside of the capacity of the archetype, capacity: [", targetArchetype.EntityCount(), "]");
        }

        // copy the entity
        std::byte* oldEntity = oldArchetype.GetEntity(oldIndex.x, oldIndex.y);
        std::byte* targetEntity = targetArchetype.GetEntity(targetIndex.x, targetIndex.y);
        *targetEntity = *oldEntity;

        Signature similarSignature = oldArchetype.signature & targetArchetype.signature;

        // destruct the desination, move into the destination, then destruct the being moved from
        ForEachComponentBit(similarSignature, [&](const ComponentData& data)
        {
            size_t oldComponentOffset = oldArchetype.componentOffsets[data.index];
            std::byte* componentSource = oldEntity + oldComponentOffset;

            size_t targetComponentOffset = targetArchetype.componentOffsets[data.index];
            std::byte* componentDestination = targetEntity + targetComponentOffset;

            data.move(componentDestination, componentSource);
            data.destruct(componentSource);
        });

    }

    void EngineManager::Archetype_SwapPop(Archetype& archetype, glm::u64vec2 chunkIndex)
    {
        if (!archetype.InBounds(chunkIndex.x, chunkIndex.y))
        {
            Gep::Log::Critical("Archetype_SwapPop() failed, the chunkIndex: [",chunkIndex,"] was outside of the capacity of the archetype, capacity: [", archetype.EntityCount(), "]");
        }

        std::byte* entityToErasePtr = archetype.GetEntity(chunkIndex.x, chunkIndex.y);
        std::byte* entityAtBackPtr = archetype.GetEntityAtBack();

        if (entityToErasePtr != entityAtBackPtr) // if they are different swap with the back and remove the back
        {
            Entity* erasedEntity = reinterpret_cast<Entity*>(entityToErasePtr);
            Entity* swappedEntity = reinterpret_cast<Entity*>(entityAtBackPtr);

            *erasedEntity = *swappedEntity;

            ForEachComponentBit(archetype.signature, [&](const ComponentData& data) 
            {
                uint64_t offset = archetype.componentOffsets[data.index];

                std::byte* componentToErase = entityToErasePtr + offset;
                std::byte* componentAtBack = entityAtBackPtr + offset;

                data.destruct(componentToErase);
                data.move(componentToErase, componentAtBack);
                data.destruct(componentAtBack);
            });

            SetArchetypeChunkIndex(*swappedEntity, chunkIndex);
        }
        else // must still call the destructors on the back components
        {
            ForEachComponentBit(archetype.signature, [&](const ComponentData& data)
            {
                uint64_t offset = archetype.componentOffsets[data.index];
                std::byte* componentAtBack = entityAtBackPtr + offset;

                data.destruct(componentAtBack);
            });
        }

        archetype.RemoveEntityAtBack();
    }

    void EngineManager::SetArchetypeChunkIndex(Entity entity, glm::u64vec2 index)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetArchetypeChunkIndex() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas.at(entity).archetypeIndex = index;
    }

    glm::u64vec2 EngineManager::GetArchetypeChunkIndex(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("GetArchetypeChunkIndex() failed, Entity: [", entity, "] does not exist");
            return { INVALID_ENTITY, INVALID_ENTITY };
        }

        return mEntityDatas.at(entity).archetypeIndex;
    }

    void EngineManager::Archetype_Erase(Entity entity, uint64_t componentIndex)
    {
        if (!EntityExists(entity))
        {
            Log::Error("Archetype_Erase() failed, Entity: [", entity, "] does not exist");
            return;
        }
        if (!ComponentIsRegistered(componentIndex))
        {
            Log::Error("Archetype_Erase() failed, component: [",componentIndex,"] is not registered");
            return;
        }

        Signature oldSignature = GetSignature(entity);
        Signature targetSignature = oldSignature;
        targetSignature.reset(componentIndex);

        if (oldSignature == targetSignature)
        {
            Log::Warning("Archetype_Erase(): When removing a component, entity: [", entity ,"] did not have the component: [", componentIndex, "]");
            return;
        }

        Archetype& oldArchetype = mArchetypes.at(oldSignature);
        glm::u64vec2 oldIndex = GetArchetypeChunkIndex(entity);

        if (targetSignature == 0) // targeting a nothing archetype, dont do any moving or allocating
        {
            Archetype_SwapPop(oldArchetype, oldIndex);
        }
        else
        {
            bool targetChunkExists = mArchetypes.contains(targetSignature);

            if (!targetChunkExists)
                Archetype_Create(targetSignature);

            Archetype& targetArchetype = mArchetypes.at(targetSignature);

            Archetype_Append(targetArchetype, entity);
            glm::u64vec2 targetIndex = GetArchetypeChunkIndex(entity);

            Archetype_Move(oldArchetype, targetArchetype, oldIndex, targetIndex);
            Archetype_SwapPop(oldArchetype, oldIndex);
        }

        if (oldArchetype.EntityCount() == 0)
            mArchetypes.erase(oldSignature);

        // update the entities signature
        Signature signature = GetSignature(entity); // gets the existing signature of the entity
        signature.reset(componentIndex);
        SetSignature(entity, signature); // sets the signature of the entity to the signature with the newly removed component
    }

    void EngineManager::Archetype_Clear(Entity entity)
    {
        Signature signature = GetSignature(entity);

        if (signature == 0) // if the entity has no components this function is a noop
            return;

        Archetype& archetype = mArchetypes.at(signature);
        glm::u64vec2 index = GetArchetypeChunkIndex(entity);

        Archetype_SwapPop(archetype, index);

        if (archetype.EntityCount() == 0)
            mArchetypes.erase(signature);
    }
}
