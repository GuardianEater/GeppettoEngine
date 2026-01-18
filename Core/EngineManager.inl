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

#define ENGINE_MANAGER EngineManager<Gep::TypeList<R...>, Gep::TypeList<C...>, Gep::TypeList<S...>>

namespace Gep
{
    template <typename... R, typename ...C, typename ...S>
    ENGINE_MANAGER::EngineManager(TypeList<R...> resourceTypes, TypeList<C...> componentTypes, TypeList<S...> systemTypes)
    {
        // needed so the unerlying vector does not reallocate.
        mComponentDatas.reserve(MAX_COMPONENTS);
        RegisterTypes(resourceTypes, componentTypes, systemTypes);
    }

    template <typename... R, typename ...C, typename ...S>
    inline void ENGINE_MANAGER::RegisterTypes(Gep::TypeList<R...> resourceTypes, Gep::TypeList<C...> componentTypes, Gep::TypeList<S...> systemTypes)
    {
        (RegisterResource<R>(), ...);
        (RegisterComponent<C>(), ...);
        (RegisterSystem<S>(), ...);

        auto systemsWithOnComponentsRegistered = systemTypes.filter<TypeHasOnComponentsRegistered>();

        systemsWithOnComponentsRegistered.for_each([&]<typename SystemType>()
        {
            this->GetSystem<SystemType>().template OnComponentsRegistered<ComponentTypes...>(componentTypes);
        });
    }


    template <typename... R, typename ...C, typename ...S>
    template<typename Func>
    inline void ENGINE_MANAGER::ExcludeFromDt(Func&& func)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        func();

        auto endTime = std::chrono::high_resolution_clock::now();
        mExcludedDeltaTime += std::chrono::duration<float>(endTime - startTime).count();
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ...ComponentTypes>
    inline Signature ENGINE_MANAGER::CreateSignature(Signature oldSignature) const
    {
        ValidateComponentTypes<ComponentTypes...>();

        (oldSignature.set(GetComponentIndex<ComponentTypes>()), ...);

        return oldSignature;
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename Func>
        requires std::invocable<Func, Entity>
    inline void ENGINE_MANAGER::ForEachChild(Entity parent, const Func& lamda) const
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

    template <typename... R, typename ...C, typename ...S>
    template<typename Func>
        requires std::invocable<Func, Entity>
    inline void ENGINE_MANAGER::ForEachSibling(Entity entity, const Func& lamda) const
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ...ComponentTypes>
    inline const std::vector<Entity>& ENGINE_MANAGER::GetEntities()
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

    template <typename... R, typename ...C, typename ...S>
    template<typename Func>
    inline void ENGINE_MANAGER::ForEachArchetype(Func&& lambda)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ...ComponentTypes>
    inline size_t ENGINE_MANAGER::CountEntities() const
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ResourceType, typename... ContructionTypes>
    inline void ENGINE_MANAGER::RegisterResource(ContructionTypes&&... pararms)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ResourceType>
    inline ResourceType& ENGINE_MANAGER::GetResource()
    {
        if (!ResourceIsRegistered<ResourceType>())
        {
            Log::Critical("GetResource() Failed, Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "] is not registered!");
        }

        return *static_cast<ResourceType*>(mResources.at(typeid(ResourceType)).get());
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ResourceType>
    inline const ResourceType& ENGINE_MANAGER::GetResource() const
    {
        if (!ResourceIsRegistered<ResourceType>())
        {
            Log::Critical("GetResource() Failed, Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "] is not registered!");
        }

        return *static_cast<ResourceType*>(mResources.at(typeid(ResourceType)).get());
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ResourceType>
    inline bool ENGINE_MANAGER::ResourceIsRegistered() const
    {
        return mResources.contains(typeid(ResourceType));
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ...ComponentTypes>
    inline bool ENGINE_MANAGER::ValidateComponentTypes() const
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline Signature ENGINE_MANAGER::GetComponentSignature() const
    {
        // TODO: remove
        uint64_t componentID = mComponentTypeToIndex.at(typeid(ComponentType));

        return mComponentDatas.at(componentID).signature;
    }

    template <typename... R, typename ...C, typename ...S>
    template <typename ComponentType>
    void ENGINE_MANAGER::RegisterComponent()
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

    template <typename... R, typename ...C, typename ...S>
    template <typename... ComponentTypes>
    void ENGINE_MANAGER::AddComponent(Entity entity, ComponentTypes... components)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ...ComponentTypes>
    inline void ENGINE_MANAGER::CopyComponent(Entity to, Entity from)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename... ComponentTypes>
    void ENGINE_MANAGER::MarkComponentForDestruction(Entity entity)
    {
        // TODO: remove mComponentTypeToIndex
        (mMarkedComponents.push_back({ entity, mComponentTypeToIndex.at(typeid(ComponentTypes)) }), ...);
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline void ENGINE_MANAGER::DestroyComponent(Entity entity)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    ComponentType& ENGINE_MANAGER::GetComponent(Entity entity)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline const ComponentType& ENGINE_MANAGER::GetComponent(Entity entity) const
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

    template <typename... R, typename ...C, typename ...S>
    template <typename... ComponentTypes>
    bool ENGINE_MANAGER::HasComponent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("HasComponent() Failed, Entity: [", entity, "] does not exist!");
        }

        Signature componentSig = CreateSignature<ComponentTypes...>();

        return ((GetSignature(entity) & componentSig) == componentSig);
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline bool ENGINE_MANAGER::ComponentIsRegistered() const
    {
        static bool registered = mComponentTypeToIndex.contains(typeid(ComponentType));

        return registered;
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename Func>
        requires std::invocable<Func, const ComponentData&>
    inline void ENGINE_MANAGER::ForEachComponent(Entity entity, Func&& lamda) const
    {
        Signature entitySignature = GetSignature(entity);
        ForEachComponentBit(entitySignature, lamda);
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename Func>
        requires std::invocable<Func, const ComponentData&>
    inline void ENGINE_MANAGER::ForEachComponentBit(Signature signature, Func&& lambda) const
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline nlohmann::json ENGINE_MANAGER::SaveComponent(Entity entity) const
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline void ENGINE_MANAGER::LoadComponent(Entity entity, const nlohmann::json& componentDataJson)
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

    template <typename... R, typename ...C, typename ...S>
    template <typename SystemType>
    void ENGINE_MANAGER::RegisterSystem()
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

    template <typename... R, typename ...C, typename ...S>
    template<typename SystemType>
    inline void ENGINE_MANAGER::SetSystemExecutionPolicy(EngineState state)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename EventType, typename FunctionType>
        requires std::invocable<FunctionType, const EventType&>
    inline void ENGINE_MANAGER::SubscribeToEvent(FunctionType function)
    {
        mEventDatas[typeid(EventType)].subscribers.emplace_back([function](const void* eventData)
        {
            function(*static_cast<const EventType*>(eventData));
        });
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename EventType, typename ClassType, typename MemberFunctionType>
        requires IsInvocableMember<ClassType, MemberFunctionType, const EventType&>
    inline void ENGINE_MANAGER::SubscribeToEvent(ClassType* object, MemberFunctionType memberFunction)
    {
        mEventDatas[typeid(EventType)].subscribers.emplace_back([object, memberFunction](const void* eventData)
        {
            (object->*memberFunction)(*static_cast<const EventType*>(eventData));
        });
    }

    template <typename... R, typename ...C, typename ...S>
    template <typename EventType>
    void ENGINE_MANAGER::SignalEvent(const EventType& eventData)
    {
        std::type_index id = typeid(EventType);

        //get the subscribers for this event type
        const auto& subscribers = mEventDatas[id].subscribers;
        for (auto& subscriber : subscribers)
        {
            subscriber(&eventData);
        }
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename SystemType>
    SystemType& ENGINE_MANAGER::GetSystem()
    {
        const uint64_t index = GetSystemIndex<SystemType>();

        return *dynamic_cast<SystemType*>(mSystems.at(index).system.get());
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename SystemType>
    uint64_t ENGINE_MANAGER::GetSystemIndex()
    {
        static const uint64_t index = mSystems.at(mSystemTypeToIndex.at(typeid(SystemType))).index;

        return index;
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    uint8_t ENGINE_MANAGER::GetComponentIndex() const
    {
        // note: there is no error check, if the below line crashes its because the component was not registered.
        // this is gross, however it needs to be done in a single line so its statically cached
        static const uint8_t index = mComponentDatas.at(mComponentTypeToIndex.at(typeid(ComponentType))).index;
        
        return index;
    }

    template <typename... R, typename ...C, typename ...S>
    template<typename... ComponentTypes>
    inline void ENGINE_MANAGER::Archetype_Append(Archetype& archetype, Entity entity, ComponentTypes... components)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ...ComponentTypes>
    inline void ENGINE_MANAGER::Archetype_Insert(Entity entity, ComponentTypes... components)
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

    template <typename... R, typename ...C, typename ...S>
    template<typename ComponentType>
    inline void ENGINE_MANAGER::OnComponentDestroyed(Gep::Entity entity)
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

    template <typename... R, typename ...C, typename ...S>
    ENGINE_MANAGER::~EngineManager()
    {
        DestroyAllEntities();

        mSystems.clear();
        mSystemsToUpdate.clear();
        mResources.clear();
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::FrameStart()
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

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::FrameEnd()
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

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::IsRunning() const
    {
        return mIsRunning;
    }

    template <typename... R, typename ...C, typename ...S>
    float ENGINE_MANAGER::GetDeltaTime() const
    {
        return mDeltaTime;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Shutdown()
    {
        mIsRunning = false;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::SetState(EngineState state)
    {
        SignalEvent(Gep::Event::EngineStateChanged{ mState, state });
        mState = state;
    }

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::IsState(EngineState state) const
    {
        return mState == state;
    }

    template <typename... R, typename ...C, typename ...S>
    EngineState ENGINE_MANAGER::GetCurrentState() const
    {
        return mState;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::SetSignature(Entity entity, Signature signature)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetSignature() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas[entity].signature = signature;
    }

    template <typename... R, typename ...C, typename ...S>
    Signature ENGINE_MANAGER::GetSignature(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("GetSignature() failed, Entity: [", entity, "] does not exist");
            return Signature{};
        }

        return mEntityDatas.at(entity).signature;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::SetName(Entity entity, const std::string& name)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetName() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas.at(entity).name = name;
    }

    template <typename... R, typename ...C, typename ...S>
    const std::string& ENGINE_MANAGER::GetName(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("GetName() failed, Entity: [", entity, "] does not exist");
        }

        return mEntityDatas.at(entity).name;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::SetUUID(Entity entity, const UUID& uuid)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetUUID() failed, Entity: [", entity, "] does not exist");
            return;
        }

        const UUID& oldid = GetUUID(entity);
        if (oldid.IsValid() && mUUIDToEntity.contains(oldid))
        {
            mUUIDToEntity.erase(oldid);
        }

        mUUIDToEntity[uuid] = entity;
        mEntityDatas.at(entity).uuid = uuid;
    }

    template <typename... R, typename ...C, typename ...S>
    const UUID& ENGINE_MANAGER::GetUUID(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Critical("GetUUID() failed, Entity: [", entity, "] does not exist");
        }

        return mEntityDatas.at(entity).uuid;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::MarkEntityForDestruction(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Log::Error("MarkEntityForDestruction() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mMarkedEntities.push_back(entity);
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::DestroyMarkedEntities()
    {
        for (const Entity entity : mMarkedEntities)
        {
            if (EntityExists(entity))
                DestroyEntity(entity);
        }

        mMarkedEntities.clear();
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::DestroyAllEntities()
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

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::DestroyEntity(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Gep::Log::Error("DestroyEntity() failed, entity: [", entity, "] does not exist");
            return;
        }

        SignalEvent(Event::EntityDestroyed{ entity });

        ForEachComponent(entity, [&](const ComponentData& componentData)
        {
            componentData.onRemove(entity);
        });

        std::vector<Entity> children = GetChildren(entity);
        for (Entity child : children)
        {
            DestroyEntity(child);
        }

        Archetype_Clear(entity);

        if (HasParent(entity))
            DetachEntity(entity);

        mUUIDToEntity.erase(mEntityDatas.at(entity).uuid);
        mEntityDatas.erase(entity);

        Log::Trace("Destroyed Entity: [", entity, "]");
    }

    template <typename... R, typename ...C, typename ...S>
    Entity ENGINE_MANAGER::CreateEntity(const std::string& name, const UUID& uuid)
    {
        Entity entity = mEntityDatas.emplace();

        SetName(entity, name);

        if (uuid.IsValid())
            SetUUID(entity, uuid);
        else
            SetUUID(entity, UUID::GenerateNew());

        SignalEvent(Event::EntityCreated{ entity });

        Log::Trace("Created Entity: [", entity, "]");

        return entity;
    }

    template <typename... R, typename ...C, typename ...S>
    Entity ENGINE_MANAGER::DuplicateEntity(Entity entity)
    {
        if (!EntityExists(entity))
        {
            Gep::Log::Error("DuplicateEntity() failed, entity: [", entity, "] does not exist");
            return INVALID_ENTITY;
        }

        nlohmann::json entityData = SaveEntity(entity);
        Entity newEntity = LoadEntity(entityData, false);
        SetUUID(newEntity, UUID::GenerateNew());

        return newEntity;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::AttachEntity(Entity parent, Entity child)
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

        if (HasParent(child))
            DetachEntity(child);

        mEntityDatas.at(parent).children.push_back(child);
        mEntityDatas.at(child).parent = parent;

        SignalEvent(Event::EntityAttached{ child, parent });
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::DetachEntity(Entity child)
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

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::HasParent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("HasParent() failed, Entity: [", entity,"] does not exist");
            return false;
        }

        if (!EntityExists(mEntityDatas.at(entity).parent))
        {
            return false;
        }

        return true;
    }

    template <typename... R, typename ...C, typename ...S>
    Entity ENGINE_MANAGER::GetParent(Entity child) const
    {
        if (!EntityExists(child))
        {
            Log::Error("GetParent() failed, Entity: [", child, "] does not exist");
            return INVALID_ENTITY;
        }

        return mEntityDatas.at(child).parent;
    }

    template <typename... R, typename ...C, typename ...S>
    std::vector<Entity> ENGINE_MANAGER::GetAncestors(Entity entity) const
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

    template <typename... R, typename ...C, typename ...S>
    Entity ENGINE_MANAGER::GetRoot(Entity child) const
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

    template <typename... R, typename ...C, typename ...S>
    size_t ENGINE_MANAGER::GetChildCount(Entity parent) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("GetChildCount() failed, Entity: [", parent, "] does not exist");
            return 0;
        }

        return mEntityDatas.at(parent).children.size();
    }

    template <typename... R, typename ...C, typename ...S>
    std::vector<Entity> ENGINE_MANAGER::GetChildren(Entity parent) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("GetChildren() failed, Entity: [", parent, "] does not exist");
            return std::vector<Entity>();
        }

        return mEntityDatas.at(parent).children;
    }

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::HasChild(Entity parent) const
    {
        if (!EntityExists(parent))
        {
            Log::Error("HasChild() failed, Entity: [", parent, "] does not exist");
            return false;
        }

        return !mEntityDatas.at(parent).children.empty();
    }

    template <typename... R, typename ...C, typename ...S>
    std::vector<Entity> ENGINE_MANAGER::GetRoots()
    {
        std::vector<Entity> rootEntities;

        const auto& entities = this->template GetEntities<>();

        for (Entity entity : entities)
        {
            if (!HasParent(entity))
            {
                rootEntities.push_back(entity);
            }
        }

        return rootEntities;
    }

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::IsEnabled(Gep::Entity entity) const
    {
        return mEntityDatas.at(entity).active;
    }

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::EntityExists(Entity entity) const
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

    template <typename... R, typename ...C, typename ...S>
    Entity ENGINE_MANAGER::FindEntity(const UUID& uuid) const
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

    template <typename... R, typename ...C, typename ...S>
    nlohmann::json ENGINE_MANAGER::SaveEntity(Entity entity) const
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

    template <typename... R, typename ...C, typename ...S>
    Entity ENGINE_MANAGER::LoadEntity(const nlohmann::json& entityJson, bool readUUID)
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

        if (!entityJson.contains("uuid"))
        {
            Gep::Log::Warning("Entity json does not contain a uuid");
        }
        else
        {
            if (readUUID)
            {
                UUID uuid = UUID::FromString(entityJson["uuid"].get<std::string>());
                if (uuid.IsValid())
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

    template <typename... R, typename ...C, typename ...S>
    const Gep::keyed_vector<ComponentData>& ENGINE_MANAGER::GetComponentDatas() const
    {
        return mComponentDatas;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::DestroyMarkedComponents()
    {
        for (const auto& [componentIndex, entity] : mMarkedComponents)
        {
            ComponentData& data = mComponentDatas.at(componentIndex);
            data.remove(entity);
        }

        mMarkedComponents.clear();
    }

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::HasComponent(uint64_t componentIndex, Entity entity) const
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

    template <typename... R, typename ...C, typename ...S>
    bool ENGINE_MANAGER::ComponentIsRegistered(uint64_t componentIndex) const
    {
        return mComponentDatas.contains(componentIndex);
    }

    template <typename... R, typename ...C, typename ...S>
    const std::unordered_map<Signature, Archetype>& ENGINE_MANAGER::GetArchetypes() const
    {
        return mArchetypes;
    }

    template <typename... R, typename ...C, typename ...S>
    const Gep::keyed_vector<SystemData>& ENGINE_MANAGER::GetSystemDatas() const
    {
        return mSystems;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Initialize()
    {
        for (const auto& index : mSystemsToUpdate)
        {
            SystemData& sd = mSystems.at(index);

            auto systemStartTime = std::chrono::high_resolution_clock::now();

            sd.system->Initialize();

            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInInitialize = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Update()
    {
        for (const auto& index : mSystemsToUpdate)
        {
            SystemData& sd = mSystems.at(index);

            auto systemStartTime = std::chrono::high_resolution_clock::now();

            if (sd.executionPolicy >= mState)
                sd.system->Update(mDeltaTime);

            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInUpdate = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Exit()
    {
        for (auto systemIt = mSystemsToUpdate.rbegin(); systemIt != mSystemsToUpdate.rend(); ++systemIt)
        {
            SystemData& sd = mSystems.at(*systemIt);

            auto systemStartTime = std::chrono::high_resolution_clock::now();

            sd.system->Exit();

            auto systemEndTime = std::chrono::high_resolution_clock::now();
            sd.timeInExit = std::chrono::duration<float>(systemEndTime - systemStartTime).count();
        }
    }

    template <typename... R, typename ...C, typename ...S>
    const SystemData& ENGINE_MANAGER::GetSystemData(uint64_t systemIndex) const
    {
        return mSystems.at(systemIndex);
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Archetype_Create(Signature signature)
    {
        if (mArchetypes.contains(signature))
        {
            Log::Critical("Cannot create archetype the given signature already exists: [", signature.to_string(), "]");
        }

        Archetype& chunk = mArchetypes[signature];

        chunk.signature = signature;
        chunk.stride += sizeof(Entity);

        ForEachComponentBit(signature, [&](const ComponentData& data)
        {
            chunk.componentOffsets.at(data.index) = chunk.stride;
            chunk.stride += data.size;
        });
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Archetype_Move(Archetype& oldArchetype, Archetype& targetArchetype, glm::u64vec2 oldIndex, glm::u64vec2 targetIndex) const
    {
        if (!oldArchetype.InBounds(oldIndex.x, oldIndex.y))
        {
            Gep::Log::Critical("Archetype_Move() failed, the oldIndex: [", oldIndex, "] was outside of the capacity of the archetype, capacity: [", oldArchetype.EntityCount(), "]");
        }
        if (!targetArchetype.InBounds(targetIndex.x, targetIndex.y))
        {
            Gep::Log::Critical("Archetype_Move() failed, the oldIndex: [", targetIndex, "] was outside of the capacity of the archetype, capacity: [", targetArchetype.EntityCount(), "]");
        }

        std::byte* oldEntity = oldArchetype.GetEntity(oldIndex.x, oldIndex.y);
        std::byte* targetEntity = targetArchetype.GetEntity(targetIndex.x, targetIndex.y);
        *targetEntity = *oldEntity;

        Signature similarSignature = oldArchetype.signature & targetArchetype.signature;

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

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Archetype_SwapPop(Archetype& archetype, glm::u64vec2 chunkIndex)
    {
        if (!archetype.InBounds(chunkIndex.x, chunkIndex.y))
        {
            Gep::Log::Critical("Archetype_SwapPop() failed, the chunkIndex: [",chunkIndex,"] was outside of the capacity of the archetype, capacity: [", archetype.EntityCount(), "]");
        }

        std::byte* entityToErasePtr = archetype.GetEntity(chunkIndex.x, chunkIndex.y);
        std::byte* entityAtBackPtr = archetype.GetEntityAtBack();

        if (entityToErasePtr != entityAtBackPtr)
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
        else
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

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::SetArchetypeChunkIndex(Entity entity, glm::u64vec2 index)
    {
        if (!EntityExists(entity))
        {
            Log::Error("SetArchetypeChunkIndex() failed, Entity: [", entity, "] does not exist");
            return;
        }

        mEntityDatas.at(entity).archetypeIndex = index;
    }

    template <typename... R, typename ...C, typename ...S>
    glm::u64vec2 ENGINE_MANAGER::GetArchetypeChunkIndex(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("GetArchetypeChunkIndex() failed, Entity: [", entity, "] does not exist");
            return { INVALID_ENTITY, INVALID_ENTITY };
        }

        return mEntityDatas.at(entity).archetypeIndex;
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Archetype_Erase(Entity entity, uint64_t componentIndex)
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

        if (targetSignature == 0)
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

        Signature signature = GetSignature(entity);
        signature.reset(componentIndex);
        SetSignature(entity, signature);
    }

    template <typename... R, typename ...C, typename ...S>
    void ENGINE_MANAGER::Archetype_Clear(Entity entity)
    {
        Signature signature = GetSignature(entity);

        if (signature == 0)
            return;

        Archetype& archetype = mArchetypes.at(signature);
        glm::u64vec2 index = GetArchetypeChunkIndex(entity);

        Archetype_SwapPop(archetype, index);

        if (archetype.EntityCount() == 0)
            mArchetypes.erase(signature);
    }

}
