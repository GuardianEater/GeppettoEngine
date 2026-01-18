/*****************************************************************//**
 * \file   EngineManager.inl
 * \brief  implementation for the EngineManager templates
 *
 * \author Travis Gronvold (2018tcg@gmail.com)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "EngineManager.hpp"
#include "JsonHelp.hpp"
#include "FunctionTraits.hpp"

#include <new>

namespace Gep
{
    template<typename... ResourceTypes, typename ...ComponentTypes, typename ...SystemTypes>
    inline void EngineManager::RegisterTypes(Gep::TypeList<ResourceTypes...> resourceTypes, Gep::TypeList<ComponentTypes...> componentTypes, Gep::TypeList<SystemTypes...> systemTypes)
    {
        (RegisterResource<ResourceTypes>(), ...);
        (RegisterComponent<ComponentTypes>(), ...);
        (RegisterSystem<SystemTypes>(), ...);

        auto systemsWithOnComponentsRegistered = systemTypes.filter<TypeHasOnComponentsRegistered>();

        systemsWithOnComponentsRegistered.for_each([&]<typename SystemType>()
        {
            this->GetSystem<SystemType>().template OnComponentsRegistered<ComponentTypes...>(componentTypes);
        });
    }

    template<typename Func>
    inline void EngineManager::ExcludeFromDt(Func&& func)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        func();

        auto endTime = std::chrono::high_resolution_clock::now();
        mExcludedDeltaTime += std::chrono::duration<float>(endTime - startTime).count();
    }

    template<typename ...ComponentTypes>
    inline Signature EngineManager::CreateSignature(Signature oldSignature) const
    {
        ValidateComponentTypes<ComponentTypes...>();

        (oldSignature.set(GetComponentIndex<ComponentTypes>()), ...);

        return oldSignature;
    }

    template<typename Func>
        requires std::invocable<Func, Entity>
    inline void EngineManager::ForEachChild(Entity parent, const Func& lamda) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("ForEachChild() failed, Entity: [", parent, "] does not exist");
            return;
        }

        const auto& children = mEntityDatas.at(parent).children;
        for (Entity child : children)
        {
            lamda(child);
        }
    }

    template<typename Func>
        requires std::invocable<Func, Entity>
    inline void EngineManager::ForEachSibling(Entity entity, const Func& lamda) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("ForEachSibling() failed, Entity: [", entity, "] does not exist");
            return;
        }
        Entity parent = GetParent(entity);
        if (parent == INVALID_ENTITY)
        {
            Log::Error("ForEachSibling() failed, Entity: [", entity, "] does not have a parent");
            return;
        }
        const auto& siblings = mEntityDatas.at(parent).children;
        for (Entity sibling : siblings)
        {
            if (sibling != entity)
            {
                lamda(mComponentDatas.at(sibling));
            }
        }
    }

    template<typename ...ComponentTypes>
    inline const std::vector<Entity>& EngineManager::GetEntities()
    {
        ([&]()
            {
                if (!ComponentIsRegistered<ComponentTypes>())
                    Log::Critical("GetEntities() Failed, Component: [", GetTypeInfo<ComponentTypes>().PrettyName(), "] is not registered!");
            }
        (), ...);

        static std::vector<Entity> entities;
        entities.clear();
        ForEachArchetype([&](Entity entity, ComponentTypes... components) 
        {
            entities.push_back(entity);
        });

        return entities;
    }

    template <typename T>
    concept TypeIsNotEntityConcept = !std::same_as<T, Entity>;

    template <typename T>
    struct TypeIsNotEntity : std::bool_constant<TypeIsNotEntityConcept<T>> {};

    template<typename Func>
    inline void EngineManager::ForEachArchetype(Func&& lambda)
    {
        using FuncArgsType = typename Gep::FunctionTraits<Func>::ArgumentsTypeList;

        FuncArgsType funcArgs; // potentially contains any of components, resources, or entity

        if constexpr (funcArgs.empty()) // if nothing is given in the query do nothing
            return;

        auto componentParamTypes = funcArgs.filter<TypeIsNotEntity>();

        componentParamTypes.for_all([&]<typename... ComponentTypes>() mutable
        {
            // if querying with no components iterate all entities instead
            if constexpr (componentParamTypes.empty())
            {
                for (auto [entity, data] : mEntityDatas)
                {
                    lambda(entity);
                }
                return;
            }

            Signature targetSignature = CreateSignature<ComponentTypes...>();

            for (auto& [signature, archetype] : mArchetypes)
            {
                // inside the matching-chunk branch
                if ((targetSignature & signature) == targetSignature)
                {
                    // cheap early-out
                    if (archetype.EntityCount() == 0) continue;

                    size_t stride = archetype.stride;

                    // precompute component offsets once per chunk
                    auto offsetsTuple = std::make_tuple(archetype.componentOffsets[GetComponentIndex<ComponentTypes>()]...);

                    archetype.ForEachEntity([&offsetsTuple, &lambda](std::byte* entity)
                    {
                        Entity& entityRef = *reinterpret_cast<Entity*>(entity);

                        // expand offset tuple and call lambda with entity + components
                        std::apply([&](auto... offs) 
                        {
                            // offs are offsets for ComponentTypes in the same order
                            lambda(entityRef, *reinterpret_cast<std::remove_reference_t<ComponentTypes>*>(entity + offs)...);
                        }, 
                        offsetsTuple);
                    });
                }
            }
        });
    }

    template<typename ...ComponentTypes>
    inline size_t EngineManager::CountEntities() const
    {
        // checks to see if all components are regisitered
        ([&]()
            {
                if (!ComponentIsRegistered<ComponentTypes>())
                {
                    Log::Error("ForEachArchetype() Failed, Component: [", GetTypeInfo<ComponentTypes>().PrettyName(), "] is not registered!");
                    return;
                }
            }
        (), ...);

        // if querying with no components give the count of all entities
        if constexpr (sizeof...(ComponentTypes) == 0)
        {
            return mEntityDatas.size();
        }

        Signature targetSignature = CreateSignature<ComponentTypes...>();
        size_t totalCount = 0;

        // accumulates the counts of all of the entities in all of the archetypes
        for (auto& [signature, chunk] : mArchetypes)
        {
            if ((targetSignature & signature) == targetSignature)
            {
                totalCount += chunk.entityCount;
            }
        }

        return totalCount;
    }

    template<typename ResourceType, typename... ContructionTypes>
    inline void EngineManager::RegisterResource(ContructionTypes&&... pararms)
    {
        if (ResourceIsRegistered<ResourceType>())
        {
            Log::Error("RegisterResource() Failed, Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "] is already registered!");
            return;
        }

        Log::Info("Registering Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "]...");

        std::type_index typeIndex = typeid(ResourceType);
        mResources.emplace(typeIndex, Gep::make_unique_void_ptr<ResourceType>(std::forward<ContructionTypes>(pararms)...));

        Log::Info("Registered Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "]");
    }

    template<typename ResourceType>
    inline ResourceType& EngineManager::GetResource()
    {
        if (!ResourceIsRegistered<ResourceType>())
        {
            Log::Critical("GetResource() Failed, Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "] is not registered!");
        }

        return *static_cast<ResourceType*>(mResources.at(typeid(ResourceType)).get());
    }

    template<typename ResourceType>
    inline const ResourceType& EngineManager::GetResource() const
    {
        if (!ResourceIsRegistered<ResourceType>())
        {
            Log::Critical("GetResource() Failed, Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "] is not registered!");
        }

        return *static_cast<ResourceType*>(mResources.at(typeid(ResourceType)).get());
    }

    template<typename ResourceType>
    inline bool EngineManager::ResourceIsRegistered() const
    {
        return mResources.contains(typeid(ResourceType));
    }

    template<typename ...ComponentTypes>
    inline bool EngineManager::ValidateComponentTypes() const
    {
        bool allRegistered = true;
        ([&]()
            {
                if (!ComponentIsRegistered<ComponentTypes>())
                {
                    Log::Error("Component Validation Failed, Component: [", GetTypeInfo<ComponentTypes>().Name(), "] is not registered!");
                    allRegistered = false;
                }
            }
        (), ...);
        return allRegistered;
    }

    template<typename ComponentType>
    inline Signature EngineManager::GetComponentSignature() const
    {
        // TODO: remove
        uint64_t componentID = mComponentTypeToIndex.at(typeid(ComponentType));

        return mComponentDatas.at(componentID).signature;
    }

    template <typename ComponentType>
    void EngineManager::RegisterComponent()
    {
        if (mComponentTypeToIndex.contains(typeid(ComponentType)))
        {
            Log::Error("RegisterComponent() Failed, Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] is already registered!");
            return;
        }

        Log::Info("Registering Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]...");

        const uint64_t componentID = mComponentDatas.emplace();
        ComponentData& newComponentData = mComponentDatas.at(componentID);

        newComponentData.signature.set(componentID);
        newComponentData.index = componentID;
        newComponentData.size = sizeof(ComponentType);

        newComponentData.name = GetTypeInfo<ComponentType>().PrettyName();

        newComponentData.move = [](void* to, void* from) { 
            new (to) ComponentType(std::move(*reinterpret_cast<ComponentType*>(from)));
        };

        newComponentData.destruct = [](void* ptr) { reinterpret_cast<ComponentType*>(ptr)->~ComponentType(); };

        newComponentData.add = [&](Entity entity) { AddComponent<ComponentType>(entity, ComponentType{}); };
        newComponentData.remove = [&](Entity entity) { DestroyComponent<ComponentType>(entity); };
        newComponentData.copy = [&](Entity to, Entity from) { CopyComponent<ComponentType>(to, from); };
        newComponentData.has = [&](Entity entity) { return HasComponent(newComponentData.index, entity); };

        newComponentData.onRemove = [&](Entity entity) { OnComponentDestroyed<ComponentType>(entity); };

        newComponentData.save = [&](Entity entity) { return SaveComponent<ComponentType>(entity); };
        newComponentData.load = [&](Entity entity, const nlohmann::json& componentJson) { LoadComponent<ComponentType>(entity, componentJson); };

        mComponentTypeToIndex[typeid(ComponentType)] = componentID;
        mComponentNameToIndex[newComponentData.name] = componentID;

        Log::Info("Registered Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] with id: [", componentID, "]");
    }

    template <typename... ComponentTypes>
    void EngineManager::AddComponent(Entity entity, ComponentTypes... components)
    {
        if (!EntityExists(entity))
        {
            Log::Error("AddComponent() Failed, Entity: [", entity, "] does not exist!");
            return;
        }

        Signature incomingComponents = CreateSignature<ComponentTypes...>();
        Signature currentSignature = GetSignature(entity);
        Signature similarSignature = (currentSignature & incomingComponents);

        if (similarSignature != 0)
        {
            Log::Error("AddComponent() Failed, the entity: [", entity ,"] already has the passed component");
            return;
        }

        // formats an output trace so every component that was added gets logged
        std::ostringstream ss;
        ((ss << "[" << GetTypeInfo<ComponentTypes>().PrettyName() << "]"), ...);
        Log::Trace("Adding Components: [", ss.str(), "] to entity: [", entity, "]...");

        // handle the memory of the componets
        Archetype_Insert(entity, std::forward<ComponentTypes>(components)...);

        // keeps track of the amount of components that exist
        (mComponentDatas.at(GetComponentIndex<ComponentTypes>()).count++, ...);

        // update the signature of the entity
        Signature signature = GetSignature(entity);
        Signature addedComponents = CreateSignature<ComponentTypes...>();
        SetSignature(entity, signature | addedComponents);

        (SignalEvent(Event::ComponentAdded<ComponentTypes>{ entity, GetComponent<ComponentTypes>(entity) }), ...);

        Log::Trace("Adding components completed");
    }

    template<typename ...ComponentTypes>
    inline void EngineManager::CopyComponent(Entity to, Entity from)
    {
        // Use a fold expression to iterate over the component types
        ([&]()
            {
                using ComponentType = ComponentTypes;

                // destroy the component if 'to' already has it
                if (HasComponent<ComponentType>(to))
                    DestroyComponent<ComponentType>(to);

                // if 'from' has the component, copy it to 'to'
                if (HasComponent<ComponentType>(from))
                    AddComponent<ComponentType>(to, GetComponent<ComponentType>(from));

                // otherwise, log an error
                else
                    Log::Error("Copy failed, Entity: [", from, "] does not have Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]");
            }
        (), ...);
    }

    template<typename... ComponentTypes>
    void EngineManager::MarkComponentForDestruction(Entity entity)
    {
        // TODO: remove mComponentTypeToIndex
        (mMarkedComponents.push_back({ entity, mComponentTypeToIndex.at(typeid(ComponentTypes)) }), ...);
    }

    template<typename ComponentType>
    inline void EngineManager::DestroyComponent(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("DestroyComponent() failed, Entity: [", entity, "] does not exist");
            return;
        }

        const uint64_t componentIndex = GetComponentIndex<ComponentType>();

        OnComponentDestroyed<ComponentType>(entity);

        Archetype_Erase(entity, componentIndex);
    }

    template<typename ComponentType>
    ComponentType& EngineManager::GetComponent(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Critical("GetComponent() Failed, Entity: [", entity, "] does not exist!");
        }
        if (!ComponentIsRegistered<ComponentType>())
        {
            Log::Critical("GetComponent() Failed, Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] is not registered!");
        }
        if (!HasComponent<ComponentType>(entity))
        {
            Log::Critical("GetComponent() Failed, Entity: [", entity, "] does not have Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]");
        }

        uint8_t componentIndex = GetComponentIndex<ComponentType>();                 // instant?
        Signature archetypeSignature = GetSignature(entity);                         // 1 add
        glm::u64vec2 index = GetArchetypeChunkIndex(entity);                         // 1 add
                                                                                     
        Archetype& archetype = mArchetypes.at(archetypeSignature);                   // slow: std::bitset hash
                                                                                     
        std::byte* entityPtr = archetype.GetEntity(index.x, index.y);                // 1 add 1 multiply
        std::byte* componentPtr = archetype.GetComponent(entityPtr, componentIndex); // 2 add

        return *reinterpret_cast<ComponentType*>(componentPtr);
    }

    template<typename ComponentType>
    inline const ComponentType& EngineManager::GetComponent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("GetComponent() Failed, Entity: [", entity, "] does not exist!");
        }
        if (!ComponentIsRegistered<ComponentType>())
        {
            Log::Critical("GetComponent() Failed, Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] is not registered!");
        }
        if (!HasComponent<ComponentType>(entity))
        {
            Log::Critical("GetComponent() Failed, Entity: [", entity, "] does not have Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]");
        }

        const uint64_t componentIndex = GetComponentIndex<ComponentType>();                // fast
        const Signature archetypeSignature = GetSignature(entity);                         // fast
        const glm::u64vec2 index = GetArchetypeChunkIndex(entity);                         // fast

        const Archetype& archetype = mArchetypes.at(archetypeSignature);                   // slow

        const std::byte* entityPtr = archetype.GetEntity(index.x, index.y);                // fast
        const std::byte* componentPtr = archetype.GetComponent(entityPtr, componentIndex); // fast

        return *reinterpret_cast<const ComponentType*>(componentPtr);
    }

    template <typename... ComponentTypes>
    bool EngineManager::HasComponent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("HasComponent() Failed, Entity: [", entity, "] does not exist!");
        }

        Signature componentSig = CreateSignature<ComponentTypes...>();

        return ((GetSignature(entity) & componentSig) == componentSig);
    }

    template<typename ComponentType>
    inline bool EngineManager::ComponentIsRegistered() const
    {
        static bool registered = mComponentTypeToIndex.contains(typeid(ComponentType));

        return registered;
    }

    template<typename Func>
        requires std::invocable<Func, const ComponentData&>
    inline void EngineManager::ForEachComponent(Entity entity, Func&& lamda) const
    {
        Signature entitySignature = GetSignature(entity);
        ForEachComponentBit(entitySignature, lamda);
    }

    template<typename Func>
        requires std::invocable<Func, const ComponentData&>
    inline void EngineManager::ForEachComponentBit(Signature signature, Func&& lambda) const
    {
        // loops through each bit on a signature, 
        // gets the bit position, which is also the component id 
        // and gets the component data from that id
        uint64_t sig = signature.to_ullong();
        while (sig)
        {
            const int componentID = std::countr_zero(sig);
            sig &= sig - 1; // sets the bit at componentID to 0
            lambda(mComponentDatas[componentID]);
        }
    }

    template<typename ComponentType>
    inline nlohmann::json EngineManager::SaveComponent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("SaveComponent() Failed, Entity: [", entity, "] does not exist!");
            return nlohmann::json();
        }

        const ComponentType& component = GetComponent<ComponentType>(entity);
        std::string componentName = Gep::GetTypeInfo<ComponentType>().PrettyName();

        nlohmann::json componentDataJson = nlohmann::json::object();
        const auto view = rfl::to_view(component);

        // this writes each type inside of the component
        view.apply([&](const auto& f)
        {
            std::string_view fieldName = f.name();
            const auto& value = *f.value();
            Json::WriteType(componentDataJson[fieldName], value);
        });

        nlohmann::json componentJson = nlohmann::json::object();
        componentJson["data"] = componentDataJson;
        componentJson["type"] = componentName;

        return componentJson;
    }

    template<typename ComponentType>
    inline void EngineManager::LoadComponent(Entity entity, const nlohmann::json& componentDataJson)
    {
        if (!EntityExists(entity))
        {
            Log::Error("LoadComponent() Failed, Entity: [", entity, "] does not exist!");
            return;
        }

        ComponentType component{};
        const auto view = rfl::to_view(component);

        view.apply([&](const auto& componentField)
        {
            if (componentDataJson.contains(componentField.name())) // checks if the type being read in matches the type of the variable 
            {
                try
                {
                    // reads the components variable name from the json data into the location of that variable
                    Json::ReadType(componentDataJson[componentField.name()], *componentField.value());
                }
                catch(const std::exception& e)
                {
                    const nlohmann::json& componentFieldJson = componentDataJson.at(componentField.name());
                    using ComponentFieldType = decltype(*componentField.value());

                    Gep::Log::Warning("Failed to read in a field on component: [", Gep::GetTypeInfo<ComponentType>().PrettyName(),"]. "
                                      "Attempted to read: [", componentFieldJson.type_name(), ": ", componentFieldJson.dump(), "] into: [", componentField.name(), ": ", Gep::GetTypeInfo<ComponentFieldType>().Name(), "].");
                }
            }
        });

        AddComponent<ComponentType>(entity, component);
    }

    template <typename SystemType>
    void EngineManager::RegisterSystem()
    {
        static_assert(TypeInheritsFrom<SystemType, ISystem>, "SystemType must inherit from ISystem");
        Log::Info("Registering System: [", GetTypeInfo<SystemType>().PrettyName(), "]...");

        const std::type_index typeID = typeid(SystemType);

        size_t systemIndex = mSystems.emplace();
        SystemData& sd = mSystems.at(systemIndex);

        sd.system = std::make_unique<SystemType>(*this);
        sd.name   = GetTypeInfo<SystemType>().PrettyName();
        sd.size   = sizeof(SystemType);
        sd.index  = systemIndex;
        
        mSystemTypeToIndex[typeID] = systemIndex;
        uint64_t cached = GetSystemIndex<SystemType>(); // cache the index

        mSystemsToUpdate.push_back(systemIndex);

        Log::Info("Registered System: [", sd.name, "]");
    }

    template<typename SystemType>
    inline void EngineManager::SetSystemExecutionPolicy(EngineState state)
    {
        const std::type_index typeID = typeid(SystemType);

        if (!mSystemTypeToIndex.contains(typeID))
        {
            Gep::Log::Error("Attempted to set the execution policy of a system the doesnt exist");
            return;
        }

        SystemData& sd = mSystems.at(mSystemTypeToIndex.at(typeID));

        sd.executionPolicy = state;
    }

    template<typename EventType, typename FunctionType>
        requires std::invocable<FunctionType, const EventType&>
    inline void EngineManager::SubscribeToEvent(FunctionType function)
    {
        mEventDatas[typeid(EventType)].subscribers.emplace_back([function](const void* eventData)
        {
            function(*static_cast<const EventType*>(eventData));
        });
    }

    template<typename EventType, typename ClassType, typename MemberFunctionType>
        requires IsInvocableMember<ClassType, MemberFunctionType, const EventType&>
    inline void EngineManager::SubscribeToEvent(ClassType* object, MemberFunctionType memberFunction)
    {
        mEventDatas[typeid(EventType)].subscribers.emplace_back([object, memberFunction](const void* eventData)
        {
            (object->*memberFunction)(*static_cast<const EventType*>(eventData));
        });
    }

    template <typename EventType>
    void EngineManager::SignalEvent(const EventType& eventData)
    {
        std::type_index id = typeid(EventType);

        //get the subscribers for this event type
        const auto& subscribers = mEventDatas[id].subscribers;
        for (auto& subscriber : subscribers)
        {
            subscriber(&eventData);
        }
    }

    template<typename SystemType>
    SystemType& EngineManager::GetSystem()
    {
        const uint64_t index = GetSystemIndex<SystemType>();

        return *dynamic_cast<SystemType*>(mSystems.at(index).system.get());
    }

    template<typename SystemType>
    uint64_t EngineManager::GetSystemIndex()
    {
        static const uint64_t index = mSystems.at(mSystemTypeToIndex.at(typeid(SystemType))).index;

        return index;
    }

    template<typename ComponentType>
    uint8_t EngineManager::GetComponentIndex() const
    {
        // note: there is no error check, if the below line crashes its because the component was not registered.
        // this is gross, however it needs to be done in a single line so its statically cached
        static const uint8_t index = mComponentDatas.at(mComponentTypeToIndex.at(typeid(ComponentType))).index;
        
        return index;
    }

    template<typename... ComponentTypes>
    inline void EngineManager::Archetype_Append(Archetype& archetype, Entity entity, ComponentTypes... components)
    {
        if (!EntityExists(entity))
        {
            Log::Error("Archetype_Append() Failed, Entity: [", entity, "] does not exist!");
            return;
        }

        std::byte* byteEntityDestination = archetype.AllocateEntity();

        Entity* entityDestination = reinterpret_cast<Entity*>(byteEntityDestination);
        *entityDestination = entity;

        ([&](auto& component)
        {
            uint64_t componentIndex = GetComponentIndex<ComponentTypes>();
            ComponentTypes* componentDestination = reinterpret_cast<ComponentTypes*>(archetype.GetComponent(byteEntityDestination, componentIndex));
            new (componentDestination) ComponentTypes(std::move(component));
        }
        (components), ...);

        glm::u64vec2 backIndex = archetype.GetBackEntityIndex();
        SetArchetypeChunkIndex(entity, backIndex);
    }

    template<typename ...ComponentTypes>
    inline void EngineManager::Archetype_Insert(Entity entity, ComponentTypes... components)
    {
        if (!EntityExists(entity))
        {
            Log::Error("Archetype_Insert() Failed, Entity: [", entity, "] does not exist!");
            return;
        }

        Signature oldSignature    = GetSignature(entity);
        Signature targetSignature = CreateSignature<ComponentTypes...>(oldSignature);

        bool entityHasPreviousArchetype = mArchetypes.contains(oldSignature);
        bool targetArchetypeExists      = mArchetypes.contains(targetSignature);

        if (!targetArchetypeExists)
            Archetype_Create(targetSignature);

        Archetype& targetArchetype = mArchetypes.at(targetSignature);

        if (entityHasPreviousArchetype)
        {
            Archetype& oldArchetype = mArchetypes.at(oldSignature);

            glm::u64vec2 oldIndex = GetArchetypeChunkIndex(entity);
            Archetype_Append(targetArchetype, entity, std::forward<ComponentTypes>(components)...);
            glm::u64vec2 targetIndex = GetArchetypeChunkIndex(entity);

            Archetype_Move(oldArchetype, targetArchetype, oldIndex, targetIndex);
            Archetype_SwapPop(oldArchetype, oldIndex);

            if (oldArchetype.EntityCount() == 0)
                mArchetypes.erase(oldSignature);
        }
        else
        {
            Archetype_Append(targetArchetype, entity, components...);
        }
    }

    template<typename ComponentType>
    inline void EngineManager::OnComponentDestroyed(Gep::Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("DestroyComponent() failed, Entity: [", entity, "] does not exist");
            return;
        }

        const uint64_t componentIndex = GetComponentIndex<ComponentType>();

        SignalEvent(Event::ComponentRemoved<ComponentType>{ entity, GetComponent<ComponentType>(entity) });

        mComponentDatas.at(GetComponentIndex<ComponentType>()).count--;
    }
}
