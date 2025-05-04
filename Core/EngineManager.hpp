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
    concept TypeHasOnComponentsRegisteredConcept = requires(T t, Gep::type_list<int> componentTypes)
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
        uint64_t archetypeIndex{ INVALID_ENTITY }; // the index into the archetype chunk where this entity has its components stored

        Entity parent{ INVALID_ENTITY }; // the parent of the entity, if it doesnt have a parent it is INVALID_ENTITY
        Signature signature{}; // the signature of the entity

        std::vector<Entity> children{}; // any children of the entity
    };

    // static data for components
    struct ComponentData
    {
        // info
        Signature signature{}; // the signature of the component
        uint8_t index{}; // the position of the component in the signature
        std::string name{}; // the name of the component
        size_t size{}; // the size of the component in bytes

        // useful entity functions
        std::function<bool(Entity)> has{}; // a function that checks if the entity has this component
        std::function<void(Entity)> add{}; // a function that adds this component to the given entity
        std::function<void(Entity)> remove{}; // a function that removes this component from the given entity
        std::function<void(Entity to, Entity from)> copy{}; // a function that copies this component from one entity to another

        // memory functions
        std::function<void(void* to, void* from)> move{}; // casts the given pointer to the component type and moves it to the new destination
        std::function<void(void*)> destruct{}; // casts the given pointer to the component type and calls the destructor

        // serialization functions
        std::function<nlohmann::json(Gep::Entity)> save{}; // writes this component to json
        std::function<void(Gep::Entity, const nlohmann::json&)> load{}; // adds the given component to the entity from json

        std::shared_ptr<IComponentArray> array{}; // where all of the components of this type are stored
    };

    struct EventData
    {
        uint8_t index{};
        std::deque<std::function<void(const void*)>> subscribers{};
    };

    class EngineManager
    {
    public:
        EngineManager();
        ~EngineManager();

        ///////////////////////////////////////////////////////////////////////////////////////////
        // foundational functions /////////////////////////////////////////////////////////////////

        template <typename... ComponentTypes, typename... SystemTypes>
        void RegisterTypes(Gep::type_list<ComponentTypes...> componentTypes, Gep::type_list<SystemTypes...> systemTypes);

        void Initialize();

        void FrameStart();
        void Update();
        void FrameEnd();

        // called on exit
        void Exit();

        bool Running() const;

        float GetDeltaTime() const;

        void Shutdown();


        /////////////////////////////////////////////////////////////////////////////////////////////////
        // entity functions /////////////////////////////////////////////////////////////////////////////

        Entity CreateEntity();
        Entity DuplicateEntity(Entity entity);
        void DestroyEntity(Entity entity);
        bool EntityExists(Entity entity) const;

        nlohmann::json SaveEntity(Entity entity) const;
        Entity LoadEntity(const nlohmann::json& entityJson);

        template <typename... ComponentTypes>
        Signature CreateSignature(Signature oldSignature = 0) const;
        void SetSignature(Entity entity, Signature signature);
        Signature GetSignature(Entity entity) const;

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
        std::vector<Entity> GetChildren(Entity parent) const;
        bool HasChild(Entity parent) const;

        template <typename Func>
            requires std::invocable<Func, Entity>
        void ForEachSibling(Entity entity, const Func& lamda) const; // iterates over all of the siblings of the entity

        template <typename... ComponentTypes>
        const std::vector<Entity>& GetEntities();
        std::vector<Entity> GetRootEntities();

        // iterates over entities with the given component types, also automatically gets the components from those entities
        template<typename... ComponentTypes, typename Func>
        inline void ForEachArchetype(Func&& lambda);

        // counts the amount of entities with the matching components
        template<typename... ComponentTypes>
        inline size_t CountEntities() const;


        /////////////////////////////////////////////////////////////////////////////////////////////////
        // resource functions ///////////////////////////////////////////////////////////////////////////

        template <typename ResourceType>
        void RegisterResource();

        template <typename ResourceType>
        ResourceType& GetResource();

        template <typename ResourceType>
        const ResourceType& GetResource() const;

        template <typename ResourceType>
        bool ResourceIsRegistered() const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // component functions //////////////////////////////////////////////////////////////////////////

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
        ComponentBitPos GetComponentBitPos() const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // system functions /////////////////////////////////////////////////////////////////////////////

        template <typename SystemType>
        void RegisterSystem();

        template <typename SystemType>
        void SetSystemSignature(Signature signature);



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



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // archetype helpers, should only be used inside of Primary Archetype Functions /////////////////

        // prepairs a chunk for use. extracts various type information from the signature. does not touch entityCount or data.
        void CreateArchetypeChunk(Signature signature);

        // simply append the given entity and components to the end of the given chunk
        template <typename... ComponentTypes>
        void ArchetypeChunkAppend(ArchetypeChunk& chunk, Entity entity, ComponentTypes... components);

        // shifts components that are in similar in both chunks from old to new,
        void ArchetypeChunkMove(ArchetypeChunk& oldChunk, ArchetypeChunk& newChunk, uint64_t oldChunkIndex, uint64_t newChunkIndex) const;

        // swaps the data associated with the given chunk index with the back, and removes the back of the chunk 
        void ArchetypeChunkSwapPop(ArchetypeChunk& chunk, uint64_t chunkIndex);

        void SetArchetypeChunkIndex(Entity entity, uint64_t index);
        uint64_t GetArchetypeChunkIndex(Entity entity) const;



        /////////////////////////////////////////////////////////////////////////////////////////////////
        // Primary Archetype Functions //////////////////////////////////////////////////////////////////

        // the full insertion operation. will remove an entity from its previous archetype, copying all of each previous data, and add it to the new one automatically
        template <typename... ComponentTypes>
        void ArchetypeChunkInsert(Entity entity, ComponentTypes... components);

        // the full deletion operation. will remove an entity from its previous archetype and add it to its new one automatically.
        void ArchetypeChunkErase(Entity entity, uint64_t componentIndex);

        // when a component is added,
        // get the entities current archetype, and get the entities new archetype.
        // call the mComponentInfos::Copy for all of the existing components into the new archetype
        // copy construct from the given

    private:
        // events
        std::unordered_map<std::type_index, EventData> mEventDatas; // typeindex of the event -> the functions that are subscribed to that event

        // entities
        std::vector<Entity> mMarkedEntities; // entities that are marked to be destroyed
        Gep::keyed_vector<EntityData> mEntityDatas; // maps from an entity -> all of data

        // components
        Gep::keyed_vector<ComponentData> mComponentDatas; // maps from a component type -> all of the data
        std::unordered_map<std::type_index, uint64_t> mComponentTypeToIndex; // maps a component type to its index
        std::unordered_map<std::string, uint64_t> mComponentNameToIndex; // maps a component name to its index
        ComponentBitPos mNextComponentBitPos = 0; // used for assigning bits in an entities signature
        std::vector<std::pair<uint64_t, Entity>> mMarkedComponents;   // The entity and the Entities component type ids.

        // archetypes
        std::unordered_map<Signature, ArchetypeChunk> mArchetypes; // maps the signature of an archetype to the archetype itself

        // probably need to turn this into a tree

        // systems
        std::unordered_map<uint64_t, Signature> mSystemSignatures; // the signatures of all of the systems maps the typeid of a system to its signature
        std::unordered_map<uint64_t, std::shared_ptr<ISystem>> mSystems;// maps the typeid of a system to the actual system class
        std::vector <std::shared_ptr<ISystem>> mSystemsToUpdate; // the list of systems that need to be updated

        // resources
        std::unordered_map<std::type_index, Gep::void_unique_ptr> mResources; // maps the type of a resource to the resource itself

        // dt
        float mDeltaTime = 0.016f;
        std::chrono::high_resolution_clock::time_point mFrameStartTime{};

        bool mIsRunning = true;
    };
}

#include "EngineManager.inl"
