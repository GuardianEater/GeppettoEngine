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
    template <typename ComponentType>
    void ComponentArray<ComponentType>::Insert(Entity entity, const ComponentType& component)
    {
        // maps the entity to the last element in the array
        mEntityToIndex[entity] = mLastElementIndex;

        // then maps the index to the entity
        mIndexToEntity[mLastElementIndex] = entity;

        mComponentArray[mLastElementIndex] = component;

        // an item was added so the last element needs to move over one
        mLastElementIndex++;
    }

    template <typename ComponentType>
    void ComponentArray<ComponentType>::Erase(Entity entity)
    {
        // an item is being removed so the last element moves over one
        mLastElementIndex--;
        size_t entityIndex = mEntityToIndex[entity];

        // moves the last component in the componentarray to the hole where the 'deleted' compoent was
        mComponentArray[entityIndex] = mComponentArray[mLastElementIndex];

        // this is the entity at the back of the array
        Entity entityOfLastElement = mIndexToEntity[mLastElementIndex];

        mEntityToIndex[entityOfLastElement] = entityIndex;
        mIndexToEntity[entityIndex] = entityOfLastElement;

        mEntityToIndex.erase(entity);
        mIndexToEntity.erase(entityIndex);
    }

    template <typename ComponentType>
    ComponentType& ComponentArray<ComponentType>::GetComponent(Entity entity)
    {
        // Return a reference to the entity's component
        return mComponentArray[mEntityToIndex[entity]];
    }

    template <typename ComponentType>
    void ComponentArray<ComponentType>::Event_EntityDestroyed(Entity entity)
    {
        if (mEntityToIndex.contains(entity))
        {
            Erase(entity);
        }
    }
}
