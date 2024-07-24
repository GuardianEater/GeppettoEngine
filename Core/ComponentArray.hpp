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
	struct tester { int x; int y; };

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;

		virtual void Event_EntityDestroyed(Entity entity) = 0;
	};

	template <typename ComponentType>
	class ComponentArray : public IComponentArray
	{
	public:
		ComponentArray() = default;
		~ComponentArray() override = default;

		void Insert(Entity entity, const ComponentType& component)
		{
			// maps the entity to the last element in the array
			mEntityToIndex[entity] = mLastElementIndex;

			// then maps the index to the entity
			mIndexToEntity[mLastElementIndex] = entity;

			mComponentArray[mLastElementIndex] = component;

			// an item was added so the last element needs to move over one
			mLastElementIndex++;
		}

		void Erase(Entity entity)
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

		ComponentType& GetComponent(Entity entity)
		{
			// Return a reference to the entity's component
			return mComponentArray[mEntityToIndex[entity]];
		}

		void Event_EntityDestroyed(Entity entity) override
		{
			if (mEntityToIndex.find(entity) != mEntityToIndex.end())
			{
				// Remove the entity's component if it existed
				Erase(entity);
			}
			else
			{
				std::cerr << "wasting time";
			}
		}

	private:
		size_t mLastElementIndex; // amount of items current in the component array
		std::array<ComponentType, MAX_ENTITIES> mComponentArray;

		// given an entity gives the index of the entity into the component array
		std::unordered_map<Entity, std::uint64_t> mEntityToIndex;

		// given the index into the component array gives the associated entity 
		std::unordered_map<std::uint64_t, Entity> mIndexToEntity;
	};
}
