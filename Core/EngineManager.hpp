/*****************************************************************//**
 * \file   EngineManager.hpp
 * \brief  the core manager of all of the other managers
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "UUID.hpp"

#include "ComponentArray.hpp"
#include "ISystem.hpp"
#include "Events.hpp"

#include "TypeList.hpp"

#include "Logger.hpp"
#include "TypeID.hpp"
#include "KeyedVector.hpp"
#include "Archetypes.h"

#include <span>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <nlohmann/json.hpp>

namespace Gep
{

    using void_unique_ptr = std::unique_ptr<void, void(*)(void*)>;

    template <typename T, typename... Args>
    void_unique_ptr make_unique_void_ptr(Args&&... args)
    {
        std::unique_ptr<T> ptr = std::make_unique<T>(std::forward<Args>(args)...);

        return void_unique_ptr(ptr.release(), [](void* p) { delete static_cast<T*>(p); });
    }

    template <typename T>
    concept TypeHasOnComponentsRegisteredConcept = requires(T t, TypeList<int> componentTypes)
    {
        { t.template OnComponentsRegistered<int>(componentTypes) } -> std::same_as<void>;
    };

    template <typename T, typename Func, typename... FuncArgs>
    concept IsInvocableMember = std::is_member_function_pointer_v<Func> && requires(T * obj, Func func, FuncArgs... args)
    {
        { (obj->*func)(std::forward<FuncArgs>(args)...) } -> std::same_as<void>;
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
        glm::u64vec2 archetypeIndex{ INVALID_ENTITY, INVALID_ENTITY }; // the index into the archetype chunk where this entity has its components stored
        Signature signature = 0; // the signature of the entity

        Entity parent = INVALID_ENTITY; // the parent of the entity, if it doesnt have a parent it is INVALID_ENTITY
        std::vector<Entity> children; // any children of the entity

        bool active = true; // TODO: whether or not this entity is aquired from a regular query
        std::string name = ""; // allows for user identification
        UUID uuid; // a way to allways identify this specific entity
    };

    // static data for components, one is generated for each component that is registered.
    struct ComponentData
    {
        // general info
        Signature signature = 0; // the signature of the component
        uint8_t index = 0;       // the position of the component in the signature
        std::string name = "";   // the name of the component
        size_t size = 0;         // the size of the component in bytes
        size_t count = 0;        // the amount of components that are currently used of this type

        // useful entity functions these are 
        std::function<bool(Entity)> has;                  // a function that checks if the entity has this component type
        std::function<void(Entity)> add;                  // a function that adds this component type to the given entity
        std::function<void(Entity)> remove;               // a function that removes this component type from the given entity
        std::function<void(Entity to, Entity from)> copy; // a function that copies this component type from one entity to another

        // event functions
        std::function<void(Entity)> onRemove; // this does not deallocate. Simply signals the event for component destruction component will be removed manually.

        // memory functions
        std::function<void(void* to, void* from)> move; // casts the given pointer to the component type and moves it to the new destination
        std::function<void(void*)> destruct;            // casts the given pointer to the component type and calls the destructor

        // serialization functions
        std::function<nlohmann::json(Gep::Entity)> save;              // writes this component to json
        std::function<void(Gep::Entity, const nlohmann::json&)> load; // adds the given component to the entity from json
    };

    // one is generated for each system that is registered
    struct SystemData
    {
        // general info
        std::string name = "";       // the name of the system
        size_t size = 0;             // the amount of memory this system takes up. should always be 0 but is not enforced
        uint64_t index = UINT64_MAX; // the index of this system into mSystems

        // used for dynamic profiling of the system
        float timeInFrameStart = 0.0f; // determines time spent in frame start call
        float timeInFrameEnd   = 0.0f; // determines time spent in frame end call
        float timeInUpdate     = 0.0f; // determines time spent in update call
        float timeInInitialize = 0.0f; // determines time spent in init call
        float timeInExit       = 0.0f; // determines time spent in exit call
        
        EngineState executionPolicy = EngineState::Edit; // when the system should be executed. always runs by default

        // the actual system instance.
        std::unique_ptr<ISystem> system; // point to the actual system
    };

    // static event data. data that is per event type
    struct EventData
    {
        uint8_t index{};
        std::deque<std::function<void(const void*)>> subscribers{}; // a list of all of the subscribers that will be called when this event happens
    };

    class EngineManager
    {
    public:
        EngineManager();
        ~EngineManager();

        ///////////////////////////////////////////////////////////////////////////////////////////
        // foundational functions /////////////////////////////////////////////////////////////////

        template<typename... ResourceTypes, typename ...ComponentTypes, typename ...SystemTypes>
        void RegisterTypes(Gep::TypeList<ResourceTypes...> resourceTypes, Gep::TypeList<ComponentTypes...> componentTypes, Gep::TypeList<SystemTypes...> systemTypes);

        void Initialize();

        void FrameStart();
        void Update();
        void FrameEnd();

        // called on exit
        void Exit();

        bool IsRunning() const;

        float GetDeltaTime() const;

        // times a function and removes its execution time from effecting dt. Useful for very expensive functions that may cause a spike in dt
        template <typename Func>
        void ExcludeFromDt(Func&& func);

        // begins the shutdown process of the engine
        void Shutdown();

        // changes the engine state to the passed state
        void SetState(EngineState state);

        // checks if the engine is a current state
        bool IsState(EngineState state) const;

        // returns the state of the engine
        EngineState GetCurrentState() const;

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // entity functions /////////////////////////////////////////////////////////////////////////////

        Entity CreateEntity(const std::string& name = "", const UUID& uuid = UUID{});
        Entity DuplicateEntity(Entity entity);
        void DestroyEntity(Entity entity);
        bool EntityExists(Entity entity) const;

        // will silently fail if it doesnt find it and return INVALID_ENTITY
        Entity FindEntity(const UUID& uuid) const;

        nlohmann::json SaveEntity(Entity entity) const;
        Entity LoadEntity(const nlohmann::json& entityJson, bool readUUID);

        template <typename... ComponentTypes>
        Signature CreateSignature(Signature oldSignature = 0) const;
        void SetSignature(Entity entity, Signature signature);
        Signature GetSignature(Entity entity) const;

        void SetName(Entity entity, const std::string& name);
        const std::string& GetName(Entity entity) const;

        void SetUUID(Entity entity, const UUID& uuid);
        const UUID& GetUUID(Entity entity) const;

        void MarkEntityForDestruction(Entity entity);
        void DestroyMarkedEntities();
        void DestroyAllEntities();

        void AttachEntity(Entity parent, Entity child);
        void DetachEntity(Entity child);

        bool HasParent(Entity child) const;
        Entity GetParent(Entity child) const;
        std::vector<Entity> GetAncestors(Entity child) const; // first element is the parent, last element is the root
        Entity GetRoot(Entity child) const;

        template <typename Func>
            requires std::invocable<Func, Entity>
        void ForEachChild(Entity parent, const Func& lamda) const; // iterates over all of the children of the entity

        size_t GetChildCount(Entity parent) const;
        const std::vector<Entity>& GetChildren(Entity parent) const;
        bool HasChild(Entity parent) const;

        template <typename Func>
            requires std::invocable<Func, Entity>
        void ForEachSibling(Entity entity, const Func& lamda) const; // iterates over all of the siblings of the entity

        template <typename... ComponentTypes>
        const std::vector<Entity>& GetEntities();
        std::vector<Entity> GetRoots();
        size_t GetEntityCount() const;

        // iterates over entities with the given component types, also automatically gets the components from those entities
        template<typename Func>
        inline void ForEachArchetype(Func&& lambda);

        // counts the amount of entities with the matching components
        template<typename... ComponentTypes>
        inline size_t CountEntities() const;

        // checks if the given entity is enabled
        bool IsEnabled(Gep::Entity entity) const;

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // resource functions ///////////////////////////////////////////////////////////////////////////

        template<typename ResourceType, typename... ContructionTypes>
        void RegisterResource(ContructionTypes&&... pararms);

        template <typename ResourceType>
        ResourceType& GetResource();

        template <typename ResourceType>
        const ResourceType& GetResource() const;

        template <typename ResourceType>
        bool ResourceIsRegistered() const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // component functions //////////////////////////////////////////////////////////////////////////

        template <typename... ComponentTypes>
        bool ValidateComponentTypes() const;

        template <typename ComponentType>
        Signature GetComponentSignature() const;

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

        template<typename ComponentType>
        ComponentType& GetComponent(Entity entity);
        template<typename ComponentType>
        ComponentType* TryGetComponent(Entity entity);
        template <typename ComponentType>
        const ComponentType& GetComponent(Entity entity) const;

        // if the entity has all of the listed components
        template <typename... ComponentTypes>
        bool HasComponent(Entity entity) const;
        bool HasComponent(uint64_t componentIndex, Entity entity) const;

        // checks if the given component type has been registered
        template <typename ComponentType>
        bool ComponentIsRegistered() const;

        bool ComponentIsRegistered(uint64_t componentIndex) const;

        // the lamda is required to take a const ComponentData& as a parameter and return void
        template <typename Func>
            requires std::invocable<Func, const ComponentData&>
        void ForEachComponent(Entity entity, Func&& lambda) const;

        // for each bit in a signature gets the corresponding component
        template <typename Func>
            requires std::invocable<Func, const ComponentData&>
        void ForEachComponentBit(Signature signature, Func&& lambda) const;

        template <typename ComponentType>
        nlohmann::json SaveComponent(Entity entity) const;

        template <typename ComponentType>
        void LoadComponent(Entity entity, const nlohmann::json& componentJson);

        // returns the index of the component. This functions return value will never change, it will always return the same value for each given type.
        template<typename ComponentType>
        uint8_t GetComponentIndex() const;

        const std::unordered_map<Signature, Archetype>& GetArchetypes() const;


        /////////////////////////////////////////////////////////////////////////////////////////////////
        // system functions /////////////////////////////////////////////////////////////////////////////

        // registers a system individualy, bypasses the on component registered call
        template <typename SystemType>
        void RegisterSystem();

        // gets all of the system data stored internally
        const Gep::keyed_vector<SystemData>& GetSystemDatas() const;

        // given a system as a template parameter, sets when that system should be ran, note system default to always running
        // must be called after a system is registered
        template <typename SystemType>
        void SetSystemExecutionPolicy(EngineState state);

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // event functions //////////////////////////////////////////////////////////////////////////////

        template<typename EventType, typename FunctionType>
            requires std::invocable<FunctionType, const EventType&>
        void SubscribeToEvent(FunctionType function);

        template <typename EventType, typename ClassType, typename MemberFunctionType>
            requires IsInvocableMember<ClassType, MemberFunctionType, const EventType&>
        void SubscribeToEvent(ClassType* object, MemberFunctionType memberFunction);

        // sends out the given event data to all subscribers of the event.
        template <typename EventType>
        void SignalEvent(const EventType& eventData);

    private:
        /////////////////////////////////////////////////////////////////////////////////////////////////
        // helper functions /////////////////////////////////////////////////////////////////////////////

        template<typename SystemType>
        SystemType& GetSystem();

        template<typename SystemType>
        uint64_t GetSystemIndex();

        const SystemData& GetSystemData(uint64_t systemIndex) const;


        /////////////////////////////////////////////////////////////////////////////////////////////////
        // archetype helpers, should only be used inside of Primary Archetype Functions /////////////////

        // prepairs a chunk for use. extracts various type information from the signature. does not touch entityCount or data.
        void Archetype_Create(Signature signature);

        // simply append the given entity and components to the end of the given chunk
        template <typename... ComponentTypes>
        void Archetype_Append(Archetype& chunk, Entity entity, ComponentTypes... components);

        // shifts components that are in similar in both chunks from old to new,
        void Archetype_Move(Archetype& oldArchetype, Archetype& newArchetype, glm::u64vec2 oldIndex, glm::u64vec2 newIndex) const;

        // swaps the data associated with the given chunk index with the back, and removes the back of the chunk 
        void Archetype_SwapPop(Archetype& chunk, glm::u64vec2 index);

        void SetArchetypeChunkIndex(Entity entity, glm::u64vec2 index);
        glm::u64vec2 GetArchetypeChunkIndex(Entity entity) const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Primary Archetype Functions //////////////////////////////////////////////////////////////////

        // the full insertion operation. will remove an entity from its previous archetype, copying all of each previous data, and add it to the new one automatically
        template <typename... ComponentTypes>
        void Archetype_Insert(Entity entity, ComponentTypes... components);

        // the full deletion operation. will remove an entity from its previous archetype and add it to its new one automatically.
        void Archetype_Erase(Entity entity, uint64_t componentIndex);

        // deallocates all components on an entity a destroys it
        void Archetype_Clear(Entity entity);

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Compoennt helpers 

        // does no deallocation. simply signals that this component is destroyed to engine itself.
        template <typename ComponentType>
        void OnComponentDestroyed(Gep::Entity entity);


    private:
        // events
        std::unordered_map<std::type_index, EventData> mEventDatas; // typeindex of the event -> the functions that are subscribed to that event

        // entities
        std::vector<Entity> mMarkedEntities; // entities that are marked to be destroyed
        Gep::keyed_vector<EntityData> mEntityDatas; // maps from an entity -> all of data
        std::unordered_map<UUID, Entity> mUUIDToEntity;

        // components
        Gep::keyed_vector<ComponentData> mComponentDatas; // maps from a component type -> all of the data
        std::unordered_map<std::type_index, uint8_t> mComponentTypeToIndex; // maps a component type to its index
        std::unordered_map<std::string, uint8_t> mComponentNameToIndex; // maps a component name to its index
        ComponentBitPos mNextComponentBitPos = 0; // used for assigning bits in an entities signature
        std::vector<std::pair<uint8_t, Entity>> mMarkedComponents;   // The entity and the Entities component type ids.

        // archetypes
        std::unordered_map<Signature, Archetype> mArchetypes; // maps the signature of an archetype to the archetype itself

        // systems
        std::unordered_map<std::type_index, uint64_t> mSystemTypeToIndex; // given the type of the system finds the index; always prefer GetSystemIndex()
        Gep::keyed_vector<SystemData> mSystems; // all of the systems running and the associated data
        std::vector<uint64_t> mSystemsToUpdate; // the list of systems that need to be updated, registration determines order

        // resources
        std::unordered_map<std::type_index, Gep::void_unique_ptr> mResources; // maps the type of a resource to the resource itself

        // dt
        float mDeltaTime = 0.016f; // the amount of time that passed over the course of the last frame
        float mExcludedDeltaTime = 0.0f; // the amount of time to be subtracked from delta time
        std::chrono::high_resolution_clock::time_point mFrameStartTime{}; // the time when this frame started

        // state
        EngineState mState = EngineState::None; // the current state of the engine
        bool mIsRunning = true; // whether or not the engine is currently running. Is checked first thing at the beggining of every frame
    };
}

#include "EngineManager.inl"
