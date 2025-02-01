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

#include "TypeList.hpp"

#include "Logger.hpp"
#include "TypeID.hpp"
#include "KeyedVector.hpp"

#include <span>
#include <rfl.hpp>
#include <rfl/json.hpp>

namespace Gep
{
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

    using SystemUpdateFunction = void(ISystem::*)(float);
    using SytemVoidFunction = void(ISystem::*)(float);

    // per entity data
    struct EntityData
    {
        Entity parent{ INVALID_ENTITY }; // the parent of the entity, if it doesnt have a parent it is INVALID_ENTITY
        Signature signature{}; // the signature of the entity

        std::vector<Entity> children{}; // any children of the entity
    };

    // static data for components
    struct ComponentData
    {
        Signature signature{}; // the signature of the component
        uint8_t index{}; // the position of the component in the signature
        std::string name{};

        std::function<bool(Entity)> has{}; // a function that checks if the entity has this component
        std::function<void(Entity)> add{}; // a function that adds this component to the given entity
        std::function<void(Entity)> remove{}; // a function that removes this component from the given entity
        std::function<void(Entity to, Entity from)> copy{}; // a function that copies this component from one entity to another

        std::shared_ptr<IComponentArray> array{}; // where all of the components of this type are stored
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

        void Initialize();

        void FrameStart();
        void Update(float dt);
        void FrameEnd();

        void Exit();

        bool Running() const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // entity functions /////////////////////////////////////////////////////////////////////////////

        Entity CreateEntity();
        Entity DuplicateEntity(Entity entity);
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
        
        template <typename... ComponentTypes>
        void CopyComponent(Entity to, Entity from);

        const Gep::keyed_vector<ComponentData>& GetComponentDatas() const;


        template<typename... ComponentTypes>
        void MarkComponentForDestruction(Entity entity);

        void DestroyMarkedComponents();

        template<typename ComponentType>
        void DestroyComponent(Entity entity);
        void DestroyComponent(uint64_t componentID, Entity entity);

        template<typename ComponentType>
        ComponentType& GetComponent(Entity entity);

        // if the entity has all of the listed components
        template <typename... ComponentTypes>
        bool HasComponent(Entity entity) const;
        bool HasComponent(uint64_t componentID, Entity entity) const;

        template <typename ComponentType>
        bool ComponentIsRegistered() const;

        // the lamda is required to take a const ComponentData& as a parameter and return void
        template <typename Func>
        requires std::invocable<Func, const ComponentData&>
        void ForEachComponent(Entity entity, Func lamda);



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
        std::shared_ptr<IComponentArray> GetComponentArray(uint64_t componentID);

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
        std::vector<Entity> mMarkedEntities; // entities that are marked to be destroyed
        std::unordered_map<Signature, std::vector<Entity>> mEntityGroups; // used by systems, holds all entities with a matching components
        Gep::keyed_vector<EntityData> mEntityDatas; // maps from an entity -> all of data

        // components
        Gep::keyed_vector<ComponentData> mComponentDatas; // maps from a component type -> all of the data
        ComponentBitPos mNextComponentBitPos; // used for assigning bits in an entities signature
        std::vector<std::pair<uint64_t, Entity>> mMarkedComponents;   // The entity and the Entities component type ids.
        std::unordered_map<std::type_index, uint64_t> mComponentTypeToID; // maps a component type to its id

        // systems
        std::unordered_map<uint64_t, Signature> mSystemSignatures; // the signatures of all of the systems maps the typeid of a system to its signature
        std::unordered_map<uint64_t, std::shared_ptr<ISystem>> mSystems;// maps the typeid of a system to the actual system class
        std::vector <std::shared_ptr<ISystem>> mSystemsToUpdate; // the list of systems that need to be updated

        bool mIsRunning;
    };
}

#include "EngineManager.inl"
