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

    void EngineManager::SetUUID(Entity entity, const UUID& uuid)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetUUID() failed, Entity: [", entity, "] does not exist");
            return;
        }

        // if the entity already had a uuid set, erase the old one
        const UUID& oldid = GetUUID(entity);
        if (oldid.IsValid() && mUUIDToEntity.contains(oldid))
        {
            mUUIDToEntity.erase(oldid);
        }

        mUUIDToEntity[uuid] = entity;
        mEntityDatas.at(entity).uuid = uuid;
    }

    const UUID& EngineManager::GetUUID(Entity entity) const
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
            DestroyEntity(entity);
        }

        mMarkedEntities.clear();
    }

    void EngineManager::DestroyAllEntities()
    {
        Gep::Log::Trace("Destroying all entities...");

        for (const auto& [entity, entityData] : mEntityDatas)
        {
            MarkEntityForDestruction(entity);
        }

        DestroyMarkedEntities();

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

        SignalEvent(Event::EntityDestroyed{ entity });

        ForEachComponent(entity, [&](const ComponentData& componentData)
        {
            componentData.remove(entity);
        });

        // note: this has to be a vector by value because detach changes the underlying storage
        std::vector<Entity> children = GetChildren(entity);
        for (Entity child : children)
        {
            DetachEntity(child);
        }
        
        if (HasParent(entity))
            DetachEntity(entity);

        mUUIDToEntity.erase(mEntityDatas.at(entity).uuid); // makesure the uuid is unbound to this entity when destroyed
        mEntityDatas.erase(entity);

        Log::Trace("Destroyed Entity: [", entity, "]");
    }

    Entity EngineManager::CreateEntity(const std::string& name, const UUID& uuid)
    {
        Entity entity = mEntityDatas.emplace();

        SetName(entity, name);

        if (uuid.IsValid()) // allows construction of an entity without a predefined uuid, will just generate one instead
            SetUUID(entity, uuid);
        else
            SetUUID(entity, UUID::GenerateNew());

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
        Entity newEntity = LoadEntity(entityData);
        SetUUID(newEntity, UUID::GenerateNew());

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

    Entity EngineManager::FindEntity(const UUID& uuid) const
    {
        if (!uuid.IsValid()) 
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
        entityJson["uuid"] = GetUUID(entity).ToString();
        entityJson["name"] = GetName(entity);

        return entityJson;
    }

    Entity EngineManager::LoadEntity(const nlohmann::json& entityJson)
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
                Gep::Entity child = LoadEntity(childJson);
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
            UUID uuid = UUID::FromString(entityJson["uuid"].get<std::string>());
            if (uuid.IsValid())
                SetUUID(entity, uuid);
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

    const Gep::keyed_vector<ComponentData>& EngineManager::GetComponentDatas() const
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

    const std::unordered_map<Signature, ArchetypeChunk>& EngineManager::GetArchetypes() const
    {
        return mArchetypes;
    }

    const Gep::keyed_vector<SystemData>& EngineManager::GetSystemDatas() const
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

    void EngineManager::CreateArchetypeChunk(Signature signature)
    {
        if (mArchetypes.contains(signature))
        {
            Log::Critical("Cannot create chunk the given signature already exists: [", signature.to_string(), "]");
        }

        ArchetypeChunk& chunk = mArchetypes[signature];

        chunk.signature = signature;
        chunk.stride += sizeof(Entity); // size of the entity

        ForEachComponentBit(signature, [&](const ComponentData& data)
        {
            chunk.componentOffsets.at(data.index) = chunk.stride;
            chunk.stride += data.size; // size of each component
        });
    }

    void EngineManager::ArchetypeChunkMove(ArchetypeChunk& oldChunk, ArchetypeChunk& targetChunk, uint64_t oldChunkIndex, uint64_t targetChunkIndex) const
    {
        if (oldChunkIndex >= oldChunk.entityCount)
        {
            Gep::Log::Critical("ArchetypeChunkMove() failed, the oldChunkIndex: [", oldChunkIndex, "] was outside of the capacity of the chunk, capacity: [", oldChunk.entityCount, "]");
        }
        if (targetChunkIndex >= targetChunk.entityCount)
        {
            Gep::Log::Critical("ArchetypeChunkMove() failed, the oldChunkIndex: [", targetChunkIndex, "] was outside of the capacity of the chunk, capacity: [", targetChunk.entityCount, "]");
        }

        // copy the entity
        uint8_t* oldEntity = oldChunk.data.data() + (oldChunkIndex * oldChunk.stride);
        uint8_t* targetEntity = targetChunk.data.data() + (targetChunkIndex * targetChunk.stride);
        *targetEntity = *oldEntity;

        Signature similarSignature = oldChunk.signature & targetChunk.signature;

        // destruct the desination, move into the destination, then destruct the being moved from
        ForEachComponentBit(similarSignature, [&](const ComponentData& data)
        {
            size_t oldComponentOffset = oldChunk.componentOffsets[data.index];
            uint8_t* componentSource = oldEntity + oldComponentOffset;

            size_t targetComponentOffset = targetChunk.componentOffsets[data.index];
            uint8_t* componentDestination = targetEntity + targetComponentOffset;

            data.move(componentDestination, componentSource);
            data.destruct(componentSource);
        });

    }

    void EngineManager::ArchetypeChunkSwapPop(ArchetypeChunk& chunk, uint64_t chunkIndex)
    {
        if (chunkIndex >= chunk.entityCount)
        {
            Gep::Log::Critical("ArchetypeChunkSwapPop() failed, the chunkIndex: [",chunkIndex,"] was outside of the capacity of the chunk, capacity: [", chunk.entityCount, "]");
        }

        --chunk.entityCount;

        uint8_t* entityToErasePtr = chunk.data.data() + (chunkIndex * chunk.stride);
        uint8_t* entityAtBackPtr = chunk.data.data() + (chunk.entityCount * chunk.stride);


        if (entityToErasePtr != entityAtBackPtr) // if they are different swap with the back and remove the back
        {
            Entity* erasedEntity = reinterpret_cast<Entity*>(entityToErasePtr);
            Entity* swappedEntity = reinterpret_cast<Entity*>(entityAtBackPtr);

            *erasedEntity = *swappedEntity;

            ForEachComponentBit(chunk.signature, [&](const ComponentData& data) 
            {
                uint64_t offset = chunk.componentOffsets[data.index];

                uint8_t* componentToErase = entityToErasePtr + offset;
                uint8_t* componentAtBack = entityAtBackPtr + offset;

                data.destruct(componentToErase);
                data.move(componentToErase, componentAtBack);
                data.destruct(componentAtBack);
            });

            SetArchetypeChunkIndex(*swappedEntity, chunkIndex);
        }
        else // must still call the destructors on the back components
        {
            ForEachComponentBit(chunk.signature, [&](const ComponentData& data)
            {
                uint64_t offset = chunk.componentOffsets[data.index];
                uint8_t* componentAtBack = entityAtBackPtr + offset;

                data.destruct(componentAtBack);
            });
        }

        chunk.data.resize(chunk.data.size() - chunk.stride);
    }

    void EngineManager::SetArchetypeChunkIndex(Entity entity, uint64_t index)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetArchetypeChunkIndex() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas.at(entity).archetypeIndex = index;
    }

    uint64_t EngineManager::GetArchetypeChunkIndex(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("GetArchetypeChunkIndex() failed, Entity: [", entity, "] does not exist");
            return INVALID_ENTITY;
        }

        return mEntityDatas.at(entity).archetypeIndex;
    }

    void EngineManager::ArchetypeChunkErase(Entity entity, uint64_t componentIndex)
    {
        if (!EntityExists(entity))
        {
            Log::Error("ArchetypeChunkErase() failed, Entity: [", entity, "] does not exist");
            return;
        }
        if (!ComponentIsRegistered(componentIndex))
        {
            Log::Error("ArchetypeChunkErase() failed, component: [",componentIndex,"] is not registered");
            return;
        }

        Signature oldSignature = GetSignature(entity);
        Signature targetSignature = oldSignature;
        targetSignature.reset(componentIndex);

        if (oldSignature == targetSignature)
        {
            Log::Warning("ArchetypeChunkErase(): When removing a component, entity: [", entity ,"] did not have the component: [", componentIndex, "]");
            return;
        }

        ArchetypeChunk& oldChunk = mArchetypes.at(oldSignature);
        uint64_t oldChunkIndex = GetArchetypeChunkIndex(entity);

        if (targetSignature == 0) // targeting a nothing archetype, dont do any moving or allocating
        {
            ArchetypeChunkSwapPop(oldChunk, oldChunkIndex);
        }
        else
        {
            bool targetChunkExists = mArchetypes.contains(targetSignature);

            if (!targetChunkExists)
                CreateArchetypeChunk(targetSignature);

            ArchetypeChunk& targetChunk = mArchetypes.at(targetSignature);

            ArchetypeChunkAppend(targetChunk, entity);
            uint64_t targetChunkIndex = GetArchetypeChunkIndex(entity);

            ArchetypeChunkMove(oldChunk, targetChunk, oldChunkIndex, targetChunkIndex);
            ArchetypeChunkSwapPop(oldChunk, oldChunkIndex);
        }

        if (oldChunk.entityCount == 0)
            mArchetypes.erase(oldSignature);
    }
}
