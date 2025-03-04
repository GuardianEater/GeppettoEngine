/*****************************************************************//**
 * \file   EngineManager.inl
 * \brief  implementation for the EngineManager templates
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   January 2025
 *********************************************************************/

#pragma once

#include "EngineManager.hpp"
#include "JsonHelp.hpp"

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
    inline const std::vector<Entity>& EngineManager::GetEntities() const
    {
        ([&]()
        {
            if (!ComponentIsRegistered<ComponentTypes>())
                Log::Critical("GetEntities() Failed, Component: [", GetTypeInfo<ComponentTypes>().PrettyName(), "] is not registered!");
        }
        (), ...);

        Signature groupSignature;

        // uses folding to create a signature from the arg list
        ((groupSignature.set(GetComponentBitPos<ComponentTypes>())), ...);

        if (!mEntityGroups.contains(groupSignature))
        {
            Log::Critical("GetEntities() Failed, EntityGroup: [", groupSignature, "] does not exist!");
        }

        return mEntityGroups.at(groupSignature);
    }

    template<typename ResourceType>
    inline void EngineManager::RegisterResource()
    {
        if (ResourceIsRegistered<ResourceType>())
        {
            Log::Error("RegisterResource() Failed, Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "] is already registered!");
            return;
        }

        Log::Info("Registering Resource: [", GetTypeInfo<ResourceType>().PrettyName(), "]...");

        std::type_index typeIndex = typeid(ResourceType);
        mResources.emplace(typeIndex, Gep::make_unique_void_ptr<ResourceType>());

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
        if (ComponentIsRegistered<ComponentType>())
        {
            Log::Error("RegisterComponent() Failed, Component: [", GetTypeInfo<ComponentType>().PrettyName(), "] is already registered!");
            return;
        }

        Log::Info("Registering Component: [", GetTypeInfo<ComponentType>().PrettyName(), "]...");

        const uint64_t componentID      = mComponentDatas.emplace();
        ComponentData& newComponentData = mComponentDatas.at(componentID);

        newComponentData.signature.set(componentID);
        newComponentData.index = componentID;
        newComponentData.name   = GetTypeInfo<ComponentType>().PrettyName();
        newComponentData.array  = std::make_shared<ComponentArray<ComponentType>>();
        newComponentData.add    = [&](Entity entity) { AddComponent<ComponentType>(entity, ComponentType{}); };
        newComponentData.remove = [&](Entity entity) { DestroyComponent(newComponentData.index, entity); };
        newComponentData.copy   = [&](Entity to, Entity from) { CopyComponent<ComponentType>(to, from); };
        newComponentData.has    = [&](Entity entity) { return HasComponent(newComponentData.index, entity); };
        newComponentData.save   = [&](Entity entity) { return SaveComponent<ComponentType>(entity); };
        newComponentData.load   = [&](Entity entity, const nlohmann::json& componentJson) { LoadComponent<ComponentType>(entity, componentJson); };

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

            GetComponentArray<ComponentType>()->insert(entity, component);

            Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
            entitySignature.set(GetComponentBitPos<ComponentType>()); // updates the entities signature with the id of the componet

            // creates an entity group if it doesnt exist
            //mEntityGroups[entitySignature].insert(entity);

            SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly added component

            SignalEvent(Event::ComponentAdded<ComponentType>{ entity });

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
        // TODO: remove mComponentTypeToIndex
        (mMarkedComponents.push_back({ entity, mComponentTypeToIndex.at(typeid(ComponentTypes)) }), ...);
    }

    template<typename ComponentType>
    inline void EngineManager::DestroyComponent(Entity entity)
    {
        // TODO: remove
        const uint64_t componentID = mComponentTypeToIndex.at(typeid(ComponentType));

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

        return GetComponentArray<ComponentType>()->at(entity);
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

        return GetComponentArray<ComponentType>()->at(entity);
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
        return mComponentTypeToIndex.contains(typeid(ComponentType));

        //return mComponentDatas.contains(componentID);
    }

    template<typename Func>
    requires std::invocable<Func, const ComponentData&>
    inline void EngineManager::ForEachComponent(Entity entity, Func lamda) const
    {
        Signature entitySignature = GetSignature(entity);
        while (entitySignature.any())
        {
            const size_t componentID = _tzcnt_u64(entitySignature.to_ullong());
            entitySignature.reset(componentID);

            lamda(mComponentDatas.at(componentID));
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
            Json::WriteType(componentDataJson, f.name(), *f.value());
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

        view.apply([&](const auto& f)
        {
            if (componentDataJson.contains(f.name()))
            {
                Json::ReadType(componentDataJson, f.name(), *f.value());
            }
        });

        AddComponent<ComponentType>(entity, component);
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

    template<typename EventType, typename FunctionType>
    requires std::invocable<FunctionType, const EventType&>
    inline void EngineManager::SubscribeToEvent(FunctionType function)
    {
        mEventDatas[typeid(EventType)].subscribers.emplace_back([function](const Gep::void_unique_ptr& eventData)
        {
            function(*static_cast<EventType*>(eventData.get()));
        });
    }

    template<typename EventType, typename ClassType, typename MemberFunctionType>
    requires IsInvocableMember<ClassType, MemberFunctionType, const EventType&>
    inline void EngineManager::SubscribeToEvent(ClassType* object, MemberFunctionType memberFunction)
    {
        mEventDatas[typeid(EventType)].subscribers.emplace_back([object, memberFunction](const Gep::void_unique_ptr& eventData)
        {
            (object->*memberFunction)(*static_cast<EventType*>(eventData.get()));
        });
    }

    template <typename EventType>
    void EngineManager::SignalEvent(const EventType& eventData)
    {
        mEventQueue.emplace_back(typeid(EventType), make_unique_void_ptr<EventType>(eventData));
    }

    template <typename ComponentType>
    std::shared_ptr<ComponentArray<ComponentType>> EngineManager::GetComponentArray()
    {
        // TODO: remove
        const uint64_t componentID = mComponentTypeToIndex.at(typeid(ComponentType));

        return std::static_pointer_cast<ComponentArray<ComponentType>>(GetComponentArray(componentID));
    }

    template<typename ComponentType>
    inline const std::shared_ptr<ComponentArray<ComponentType>> EngineManager::GetComponentArray() const
    {
        const uint64_t componentID = mComponentTypeToIndex.at(typeid(ComponentType));

        return std::static_pointer_cast<ComponentArray<ComponentType>>(GetComponentArray(componentID));
    }

    template<typename SystemType>
    SystemType& EngineManager::GetSystem()
    {
        const uint64_t typeID = typeid(SystemType).hash_code();
        return *std::static_pointer_cast<SystemType>(mSystems.at(typeID));
    }

    template<typename ComponentType>
    ComponentBitPos EngineManager::GetComponentBitPos() const
    {
        // TODO: remove
        const uint64_t componentID = mComponentTypeToIndex.at(typeid(ComponentType));

        return mComponentDatas.at(componentID).index;
    }
}
