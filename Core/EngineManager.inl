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
        return Signature().set(GetComponentBitPos<ComponentType>());
    }

    template <typename ComponentType>
    void EngineManager::RegisterComponent()
    {
        static_assert(TypeIsReflectable<ComponentType>, "ComponentType must be a trivial struct");

        Log::Info("Registering Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]...");

        const uint64_t typeID = typeid(ComponentType).hash_code();

        mComponentIDs[typeID] = mNextComponentID;

        mComponentArrays[typeID] = std::make_shared<ComponentArray<ComponentType>>();

        ++mNextComponentID;

        Log::Info("Registered Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]");
    }

    template <typename... ComponentTypes>
    void EngineManager::AddComponent(Entity entity, ComponentTypes... components)
    {
        // formats the component types into a string
        std::ostringstream oss; ((oss << GetTypeInfo<ComponentTypes>().PrettyName() << ", "), ...);
        std::string componentsString = oss.str().substr(0, oss.str().size() - 2);

        Log::Trace("Adding Component(s): [", componentsString, "] to entity:", entity, "...");

        (GetComponentArray<ComponentTypes>()->Insert(entity, components), ...);

        Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
        ((entitySignature.set(GetComponentBitPos<ComponentTypes>())), ...); // updates the entities signature with the id of the componet

        // creates an entity group if it doesnt exist
        //mEntityGroups[entitySignature].insert(entity);

        SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly added component

        Log::Trace("Added Component(s)");
    }

    template<typename... ComponentTypes>
    void EngineManager::MarkComponentForDestruction(Entity entity)
    {
        (mMarkedComponents.push_back({ entity, typeid(ComponentTypes).hash_code() }), ...);
    }

    template<typename ComponentType>
    ComponentType& EngineManager::GetComponent(Entity entity)
    {
        return GetComponentArray<ComponentType>()->GetComponent(entity);
    }

    template <typename... ComponentTypes>
    bool EngineManager::HasComponent(const Entity entity) const
    {
        Signature componentSig;
        (componentSig.set(GetComponentBitPos<ComponentTypes>()), ...); // checks if an entity has all of the given components
        return ((GetSignature(entity) & componentSig) == componentSig);
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
        const std::uint64_t typeID = typeid(ComponentType).hash_code();

        return std::static_pointer_cast<ComponentArray<ComponentType>>(mComponentArrays.at(typeID));
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
        const std::uint64_t typeID = typeid(ComponentType).hash_code();

        return mComponentIDs.at(typeID);
    }
}
