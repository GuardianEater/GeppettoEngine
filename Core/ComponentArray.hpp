/*****************************************************************//**
 * \file   ComponentArray.hpp
 * \brief  An array that is always packed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

namespace Gep
{
    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;

        virtual void Event_EntityDestroyed(Entity entity) = 0;
        virtual void Erase(Entity entity) = 0;
    };

    template <typename ComponentType>
    class ComponentArray : public IComponentArray
    {
    public:
        ComponentArray() noexcept;
        ~ComponentArray() override = default;

        // adds a component to the array attached to the entity
        void Insert(Entity entity, const ComponentType& component);

        // removes the given component from an entity
        void Erase(Entity entity) override;

        // gets a component off of an entity
        ComponentType& GetComponent(Entity entity);

        // when an entity is destroyed it needs to be removed from the component array
        void Event_EntityDestroyed(Entity entity) override;

    private:
        std::vector <ComponentType> mComponentArray{}; // the array of components

        // given an entity gives the index of the entity into the component array
        std::unordered_map<Entity, std::uint64_t> mEntityToIndex{};

        // given the index into the component array gives the associated entity 
        std::unordered_map<std::uint64_t, Entity> mIndexToEntity{};
    };
}

#include "ComponentArray.inl"