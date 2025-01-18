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
    template<typename ...ComponentTypes>
    inline std::unordered_set<Entity>& EngineManager::GetEntities()
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

    template <typename ComponentType>
    void EngineManager::RegisterComponent()
    {
        static_assert(TypeIsComponent<ComponentType>, "Attempting to register a component that is not a POD");

        const uint64_t typeID = typeid(ComponentType).hash_code();

        mComponentIDs[typeID] = mNextComponentID;

        mComponentArrays[typeID] = std::make_shared<ComponentArray<ComponentType>>();

        ++mNextComponentID;
    }

    template <typename... ComponentTypes>
    void EngineManager::AddComponent(Entity entity, ComponentTypes... components)
    {
        (GetComponentArray<ComponentTypes>()->Insert(entity, components), ...);

        Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
        ((entitySignature.set(GetComponentBitPos<ComponentTypes>())), ...); // updates the entities signature with the id of the componet

        // creates an entity group if it doesnt exist
        //mEntityGroups[entitySignature].insert(entity);

        SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly added component
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

        const uint64_t typeID = typeid(SystemType).hash_code();

        mSystems[typeID] = std::make_shared<SystemType>(*this);

        mSystemsToUpdate.push_back(mSystems.at(typeID));
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

    // initializes all systems in the order registered
    template <typename SystemType>
    void EngineManager::Initialize()
    {
        static_assert(TypeHasInitialize<SystemType>, "You passed a type to Initialize that has no Initialize function!");

        GetSystem<SystemType>().Initialize();
    }

    template<typename SystemType>
    void EngineManager::Update(float dt)
    {
        static_assert(TypeHasUpdate<SystemType>, "Attempting to update a class with no update!");

        GetSystem<SystemType>().Update(dt);
    }

    template <typename SystemType>
    void EngineManager::Exit()
    {
        static_assert(TypeHasExit<SystemType>, "You passed a type to Exit that has no Exit function!");

        GetSystem<SystemType>().Exit();
    }

    template<typename SystemType>
    void EngineManager::RenderImGui(float dt)
    {
        static_assert(TypeHasUpdate<SystemType>, "Attempting to ImGuiRender a class with no update!");

        GetSystem<SystemType>().RenderImGui(dt);
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
