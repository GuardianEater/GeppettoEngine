/*****************************************************************//**
 * \file   ComponentArray.hpp
 * \brief  An array that is always packed
 *
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include "ComponentArray.hpp"

namespace Gep
{
    template<typename ComponentType>
    inline ComponentArray<ComponentType>::ComponentArray() noexcept
    {}

    template <typename ComponentType>
    void ComponentArray<ComponentType>::insert(Entity entity, const ComponentType& component)
    {
        size_t newIndex = mComponentArray.size();
        mEntityToIndex[entity] = newIndex;
        mIndexToEntity[newIndex] = entity;
        mComponentArray.push_back(component);
    }

    template <typename ComponentType>
    void ComponentArray<ComponentType>::erase(Entity entity)
    {
        size_t indexOfRemovedEntity = mEntityToIndex[entity];
        size_t indexOfLastComponent = mComponentArray.size() - 1;

        // move the last element to the hole left by the removed element
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastComponent];

        Entity entityOfLastElement = mIndexToEntity[indexOfLastComponent];
        mEntityToIndex[entityOfLastElement] = indexOfRemovedEntity;
        mIndexToEntity[indexOfRemovedEntity] = entityOfLastElement;

        mComponentArray.pop_back();

        mEntityToIndex.erase(entity);
        mIndexToEntity.erase(indexOfLastComponent);
    }

    template <typename ComponentType>
    ComponentType& ComponentArray<ComponentType>::at(Entity entity)
    {
        // Return a reference to the entity's component
        return mComponentArray[mEntityToIndex[entity]];
    }

    template<typename ComponentType>
    inline size_t ComponentArray<ComponentType>::size() const
    {
        return mComponentArray.size();
    }
}
