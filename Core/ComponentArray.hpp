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

        virtual void erase(Entity entity) = 0;
        virtual size_t size() const = 0;
    };

    template <typename ComponentType>
    class ComponentArray : public IComponentArray
    {
    public:
        ComponentArray() noexcept;
        ~ComponentArray() override = default;

        // adds a component to the array attached to the entity
        void insert(Entity entity, const ComponentType& component);

        // removes the given component from an entity
        void erase(Entity entity) override;

        // gets a component off of an entity
        ComponentType& at(Entity entity);

        size_t size() const override;

    private:
        std::vector <ComponentType> mComponentArray{}; // the array of components

        // given an entity gives the index of the entity into the component array
        std::unordered_map<Entity, std::uint64_t> mEntityToIndex{};

        // given the index into the component array gives the associated entity 
        std::unordered_map<std::uint64_t, Entity> mIndexToEntity{};
    };
}

#include "ComponentArray.inl"