/*****************************************************************//**
 * \file   EngineManager.inl
 * \brief  implementation for the EngineManager templates
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "EngineManager.hpp"

namespace Gep
{
    template<typename ...ComponentTypes, typename ...SystemTypes>
    inline void EngineManager::RegisterTypes(Gep::type_list<ComponentTypes...> componentTypes, Gep::type_list<SystemTypes...> systemTypes)
    {
        (RegisterComponent<ComponentTypes>(), ...);
        (RegisterSystem<SystemTypes>(), ...);

        auto systemsWithOnComponentsRegistered = systemTypes.filter<TypeHasOnComponentsRegistered>();

        systemsWithOnComponentsRegistered.for_each([&]<typename SystemType>()
        {
            this->GetSystem<SystemType>().template OnComponentsRegistered<ComponentTypes...>(componentTypes);
        });
    }

    template<typename ...ComponentTypes>
    inline std::vector<Entity>& EngineManager::GetEntities()
    {
        Signature groupSignature;

        // uses folding to create a signature from the arg list
        ((groupSignature.set(GetComponentBitPos<ComponentTypes>())), ...);

#ifdef _DEBUG
        if (!mEntityGroups.contains(groupSignature))
        {
            throw std::logic_error("The given group was not registered! You Must First Register The group!");
        }
#endif // _DEBUG
        return mEntityGroups.at(groupSignature);
    }

    template<typename ComponentType>
    inline Signature EngineManager::GetComponentSignature()
    {
        // TODO: remove
        uint64_t componentID = mComponentTypeToID.at(typeid(ComponentType));

        return mComponentDatas.at(componentID).signature;
    }

    template <typename ComponentType>
    void EngineManager::RegisterComponent()
    {
        if (ComponentIsRegistered<ComponentType>())
        {
            Log::Error("RegisterComponent() Failed, Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] is already registered!");
            return;
        }

        Log::Info("Registering Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]...");

        const uint64_t componentID      = mComponentDatas.emplace();
        ComponentData& newComponentData = mComponentDatas.at(componentID);

        newComponentData.signature.set(componentID);
        newComponentData.bitPos = componentID;
        newComponentData.name   = GetTypeInfo<ComponentType>().PrettyName();
        newComponentData.array  = std::make_shared<ComponentArray<ComponentType>>();
        newComponentData.add    = [&](Entity entity) { AddComponent<ComponentType>(entity, ComponentType{}); };
        newComponentData.remove = [&](Entity entity) { DestroyComponent<ComponentType>(entity); };
        newComponentData.copy   = [&](Entity to, Entity from) { CopyComponent<ComponentType>(to, from); };

        mComponentTypeToID[typeid(ComponentType)] = componentID;

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

        ([&](const auto& component)
        {
            using ComponentType = std::decay_t<decltype(component)>;

            if (!ComponentIsRegistered<ComponentType>())
            {
                Log::Error("AddComponent() Failed, Component: [", GetTypeInfo<decltype(component)>().PrettyName(), "] is not registered!");
                return;
            }

            Log::Trace("Adding Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] to entity: [", entity, "]...");
            // formats the component types into a string

            GetComponentArray<ComponentType>()->Insert(entity, component);

            Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
            entitySignature.set(GetComponentBitPos<ComponentType>()); // updates the entities signature with the id of the componet

            // creates an entity group if it doesnt exist
            //mEntityGroups[entitySignature].insert(entity);

            SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly added component

            Log::Trace("Successfully added component: [", GetTypeInfo<ComponentType>().PrettyName(), "] to entity: [", entity, "]");
        }(components), ...);
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
        }(), ...);
    }

    template<typename... ComponentTypes>
    void EngineManager::MarkComponentForDestruction(Entity entity)
    {
        // TODO: remove mComponentTypeToID
        (mMarkedComponents.push_back({ entity, mComponentTypeToID.at(typeid(ComponentTypes)) }), ...);
    }

    template<typename ComponentType>
    inline void EngineManager::DestroyComponent(Entity entity)
    {
        // TODO: remove
        const uint64_t componentID = mComponentTypeToID.at(typeid(ComponentType));

        DestroyComponent(componentID, entity);
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

        return GetComponentArray<ComponentType>()->GetComponent(entity);
    }

    template <typename... ComponentTypes>
    bool EngineManager::HasComponent(Entity entity) const
    {
        if (!EntityExists(entity))
        {
            Log::Error("HasComponent() Failed, Entity: [", entity, "] does not exist!");
            return false;
        }

        Signature componentSig = 0;
        ((componentSig.set(GetComponentBitPos<ComponentTypes>())), ...);

        return ((GetSignature(entity) & componentSig) == componentSig);
    }

    template<typename ComponentType>
    inline bool EngineManager::ComponentIsRegistered() const
    {
        // TODO: remove
        return mComponentTypeToID.contains(typeid(ComponentType));

        //return mComponentDatas.contains(componentID);
    }

    template<typename Func>
    requires std::invocable<Func, const ComponentData&>
    inline void EngineManager::ForEachComponent(Entity entity, Func lamda)
    {
        Signature entitySignature = GetSignature(entity);
        while (entitySignature.any())
        {
            const size_t componentID = _tzcnt_u64(entitySignature.to_ullong());
            entitySignature.reset(componentID);

            lamda(mComponentDatas.at(componentID));
        }
    }

    template <typename SystemType>
    void EngineManager::RegisterSystem()
    {
        static_assert(TypeInheritsFrom<SystemType, ISystem>, "SystemType must inherit from ISystem");
        Log::Info("Registering System: [", GetTypeInfo<SystemType>().PrettyName(), "]...");

        const uint64_t typeID = typeid(SystemType).hash_code();

        mSystems[typeID] = std::make_shared<SystemType>(*this);

        mSystemsToUpdate.push_back(mSystems.at(typeID));

        Log::Info("Registered System: [", GetTypeInfo<SystemType>().PrettyName(), "]");
    }

    template <typename SystemType>
    void EngineManager::SetSystemSignature(Signature signature)
    {
        const uint64_t typeID = typeid(SystemType).hash_code();

        mSystemSignatures[typeID] = signature;
    }

    template <typename... ComponentTypes>
    void EngineManager::RegisterGroup()
    {
        Signature groupSignature;

        // uses folding to create a signature from the arg list
        ((groupSignature.set(GetComponentBitPos<ComponentTypes>())), ...);

        mEntityGroups[groupSignature];
    }

    template<typename SystemType, typename EventType, typename MemberFunctionPtr>
    void EngineManager::SubscribeToEvent(MemberFunctionPtr function)
    {
        SystemType& system = GetSystem<SystemType>(); // call member function

        GetEventFunctions<EventType>().emplace_back(std::bind(function, std::ref(system), std::placeholders::_1));
    }

    template <typename EventType>
    void EngineManager::SignalEvent(const EventType& eventData)
    {
        GetEventData<EventType>().push_back(eventData);
    }

    template <typename EventType>
    void EngineManager::StartEvent()
    {
        // the order of these for loops is preference
        for (const EventType& eventData : GetEventData<EventType>())
        {
            for (EventFunction<EventType>& eventFunction : GetEventFunctions<EventType>())
            {
                eventFunction(eventData);
            }
        }
        GetEventData<EventType>().clear();
    }

    template <typename ComponentType>
    std::shared_ptr<ComponentArray<ComponentType>> EngineManager::GetComponentArray()
    {
        // TODO: remove
        const uint64_t componentID = mComponentTypeToID.at(typeid(ComponentType));

        return std::static_pointer_cast<ComponentArray<ComponentType>>(GetComponentArray(componentID));
    }

    // keeps a lists of subscribers for each type of event
    template<typename EventType>
    std::vector<EventFunction<EventType>>& EngineManager::GetEventFunctions()
    {
        static std::vector<EventFunction<EventType>> subscribers;
        return subscribers;
    }

    template<typename SystemType>
    SystemType& EngineManager::GetSystem()
    {
        const uint64_t typeID = typeid(SystemType).hash_code();
        return *std::static_pointer_cast<SystemType>(mSystems.at(typeID));
    }

    // stores the event data for each event
    template<typename EventType>
    std::vector<EventType>& EngineManager::GetEventData()
    {
        static std::vector<EventType> eventData;
        return eventData;
    }

    template<typename ComponentType>
    ComponentBitPos EngineManager::GetComponentBitPos() const
    {
        // TODO: remove
        const uint64_t componentID = mComponentTypeToID.at(typeid(ComponentType));

        return mComponentDatas.at(componentID).bitPos;
    }
}
