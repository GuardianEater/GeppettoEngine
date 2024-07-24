/*****************************************************************//**
 * \file   Manager.hpp
 * \brief  
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

// core
#include <Core.hpp>

// project
#include <ComponentArray.hpp>

namespace Gep
{
    class ComponentManager // something that contains components
    {
    public:
        ComponentManager()
            : mComponentIDs()
            , mComponentArrays()
            , mNextComponentID(0)
        {}

        ~ComponentManager() = default;

    public:
        template <typename ComponentType>
        void Register()
        {
            const std::uint64_t typeID = typeid(ComponentType).hash_code();

            mComponentIDs[typeID] = mNextComponentID;

            mComponentArrays[typeID] = std::make_shared<ComponentArray<ComponentType>>();
            
            ++mNextComponentID;
        }

        template<typename ComponentType>
        ComponentID GetComponentID() 
        {
            const std::uint64_t typeID = typeid(ComponentType).hash_code();

            return mComponentIDs[typeID];
        }

        template<typename ComponentType>
        void AddComponent(Entity entity, const ComponentType& component)
        {
            GetComponentArray<ComponentType>()->Insert(entity, component);
        }

        template<typename ComponentType>
        void RemoveComponent(Entity entity)
        {
            GetComponentArray<ComponentType>()->Erase(entity);
        }

        template<typename ComponentType>
        ComponentType& GetComponent(Entity entity)
        {
            return GetComponentArray<ComponentType>()->GetComponent(entity);
        }

        void Event_EntityDestroyed(Entity entity)
        {
            for (const auto& [id, componentArray] : mComponentArrays)
            {
                componentArray->Event_EntityDestroyed(entity);
            }
        }

    private:
        // maps the type id to the component
        std::unordered_map<std::uint64_t, ComponentID> mComponentIDs;

        // maps the 
        std::unordered_map<std::uint64_t, std::shared_ptr<IComponentArray>> mComponentArrays;

        ComponentID mNextComponentID;

    private:

        template <typename ComponentType>
        std::shared_ptr<ComponentArray<ComponentType>> GetComponentArray()
        {
            const std::uint64_t typeID = typeid(ComponentType).hash_code();

            return std::static_pointer_cast<ComponentArray<ComponentType>>(mComponentArrays[typeID]);
        }
    };
}