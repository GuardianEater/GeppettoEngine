/*****************************************************************//**
 * \file   HeirarchyStorage.hpp
 * \brief  the is how entity ids are stored for the editor heirarchy
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

namespace Client
{
	struct HierarchyEntity
	{
		Gep::Entity entity = Gep::INVALID_ENTITY;
		bool nodeOpen = false;
		std::vector<HierarchyEntity> children;
	};

	template <typename EntityType>
	class orderable_entity_storage
	{
	public:
		void insert(Gep::Entity rootEntity, const EntityType& edt);
		void insert(Gep::Entity existingEntity, Gep::Entity newEntity, const EntityType& edt);

		void erase(Gep::Entity entity);

		EntityType& at(Gep::Entity entity);

	private:
		struct Node
		{
			Gep::Entity entity;
			EntityType data;
			std::vector<Node> children;
		};

		std::vector<Node> mRootEntities;
	};
}
