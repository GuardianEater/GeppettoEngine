/*****************************************************************//**
 * \file   EngineManager.hpp
 * \brief  the core manager of all of the other managers
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include "Core.hpp"

#include "ComponentArray.hpp"
#include "ISystem.hpp"
#include "Events.hpp"

#include "Application.hpp"
#include "TypeList.hpp"

#include "Logger.hpp"
#include "TypeID.hpp"

#include <rfl.hpp>
#include <rfl/json.hpp>

namespace Gep
{
    template <typename T>
    concept TypeHasInitialize = requires(T t)
    {
        { t.Initialize() } -> std::same_as<void>;
    };

    template <typename T>
    concept TypeHasUpdate = requires(T t, float dt)
    {
        { t.Update(dt) } -> std::same_as<void>;
    };

    template <typename T>
    concept TypeHasExit = requires(T t)
    {
        { t.Exit() } -> std::same_as<void>;
    };

    template <typename T>
    concept TypeHasOnComponentsRegisteredConcept = requires(T t, Gep::type_list<int> componentTypes)
    {
        { t.template OnComponentsRegistered<int>(componentTypes) } -> std::same_as<void>;
    };

    template <typename T>
    struct TypeHasOnComponentsRegistered : std::false_type {};
    template <typename T>
    requires TypeHasOnComponentsRegisteredConcept<T>
    struct TypeHasOnComponentsRegistered<T> : std::true_type {};

    template <typename T, typename Base>
    concept TypeInheritsFrom = std::is_base_of_v<Base, T>;

    template <typename T>
    concept TypeIsComponent = std::is_trivial<T>::value && std::is_standard_layout<T>::value;

    template <typename T>
    concept TypeIsSystem = std::is_base_of<ISystem, T>::value;

    /// doesnt seems to work...
    template <typename T>
    concept TypeIsReflectable = requires(T t)
    {
        { rfl::json::write(t) } -> std::same_as<std::string>;
    };

    using SystemUpdateFunction = void(ISystem::*)(float);
    using SytemVoidFunction = void(ISystem::*)(float);


    struct EntityData
    {
        Entity parent{};
        Signature signature{};

        std::vector<Entity> children{};
    };

    class EngineManager
    {
    public:
        EngineManager();

        ///////////////////////////////////////////////////////////////////////////////////////////
        // foundational functions /////////////////////////////////////////////////////////////////

        template <typename... ComponentTypes, typename... SystemTypes>
        void RegisterTypes(Gep::type_list<ComponentTypes...> componentTypes, Gep::type_list<SystemTypes...> systemTypes);

        template <typename... ComponentTypes>
        void RegisterGroup();

        void Start();
        void Initialize();

        void FrameStart();
        void Update(float dt);
        void FrameEnd();

        void Exit();
        void End();

        bool Running() const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // entity functions /////////////////////////////////////////////////////////////////////////////

        Entity CreateEntity();
        void DestroyEntity(Entity entity);
        bool EntityExists(Entity entity) const;

        void SetSignature(Entity entity, Signature signature);
        Signature GetSignature(Entity entity) const;

        void MarkEntityForDestruction(Entity entity);
        void DestroyMarkedEntities();

        void AttachEntity(Entity parent, Entity child);
        void DetachEntity(Entity child);

        bool HasParent(Entity entity) const;
        Entity GetParent(Entity child);

        std::vector<Entity> GetSiblings(Entity entity);
        std::vector<Entity> GetChildren(Entity parent);

        template <typename... ComponentTypes>
        std::vector<Entity>& GetEntities();

        

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // component functions //////////////////////////////////////////////////////////////////////////

        std::vector<Signature> GetComponentSignatures(Entity entity);

        template <typename ComponentType>
        Signature GetComponentSignature();

        template <typename ComponentType>
        void RegisterComponent();

        template <typename... ComponentTypes>
        void AddComponent(Entity entity, ComponentTypes... components);

        template<typename... ComponentTypes>
        void MarkComponentForDestruction(Entity entity);

        void DestroyMarkedComponents();

        void DestroyComponent(Entity entity, uint64_t component);

        template<typename ComponentType>
        ComponentType& GetComponent(Entity entity);

        // if the entity has all of the listed components
        template <typename... ComponentTypes>
        bool HasComponent(const Entity entity) const;

        bool HasComponent(const Entity entity, const uint64_t componentID) const;

        template <typename ComponentType>
        bool ComponentIsRegistered() const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // system functions /////////////////////////////////////////////////////////////////////////////

        template <typename SystemType>
        void RegisterSystem();

        template <typename SystemType>
        void SetSystemSignature(Signature signature);



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // event functions //////////////////////////////////////////////////////////////////////////////

        template<typename SystemType, typename EventType, typename MemberFunctionPtr>
        void SubscribeToEvent(MemberFunctionPtr function);

        template <typename EventType>
        void SignalEvent(const EventType& eventData);

        template <typename EventType>
        void StartEvent();

    private:
        std::shared_ptr<IComponentArray> GetComponentArray(uint64_t typeID);

        template <typename ComponentType>
        std::shared_ptr<ComponentArray<ComponentType>> GetComponentArray();

        // keeps a lists of subscribers for each type of event
        template<typename EventType>
        std::vector<EventFunction<EventType>>& GetEventFunctions();

        template<typename SystemType>
        SystemType& GetSystem();

        // stores the event data for each event
        template<typename EventType>
        std::vector<EventType>& GetEventData();

        template<typename ComponentType>
        ComponentBitPos GetComponentBitPos() const;

    private:
        // events
        std::vector<std::shared_ptr<Event::IEvent>> mEventQueue; // All events that need to happen

        // entities
        std::vector<Entity> mAvailableEntities; // list of unused entity ids
        std::vector<Entity> mMarkedEntities; // entities that are marked to be destroyed
        std::unordered_map<Signature, std::vector<Entity>> mEntityGroups; // used by systems, holds all entities with a matching components
        std::unordered_map<Entity, EntityData> mEntityDatas; // maps from an entity -> all of data

        // components
        ComponentBitPos mNextComponentID; // used for assigning bits in an entities signature
        std::unordered_map<uint64_t, ComponentBitPos> mComponentIDs; // maps the type id to the component bit position
        std::vector<std::pair<Entity, uint64_t>> mMarkedComponents;   // The entity and the Entities component type ids.
        std::unordered_map<uint64_t, std::shared_ptr<IComponentArray>> mComponentArrays; // maps the component typeid to an array of the component

        // systems
        std::unordered_map<uint64_t, Signature> mSystemSignatures; // the signatures of all of the systems maps the typeid of a system to its signature
        std::unordered_map<uint64_t, std::shared_ptr<ISystem>> mSystems;// maps the typeid of a system to the actual system class

        std::vector <std::shared_ptr<ISystem>> mSystemsToUpdate; // the list of systems that need to be updated

        Application mApplication;

        bool mIsRunning;
    };
}

#include "EngineManager.inl"
