/*****************************************************************//**
 * \file   EditorResource.hpp
 * \brief  various functions to allow interacting with the editor. 
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"

namespace Client
{
	class EditorResource
	{
	public:
		void SmartSelectEntity(Gep::Entity entity);

		void SelectEntity(Gep::Entity entity);
		void DeselectEntity(Gep::Entity entity);

		void SelectAll(std::span<const Gep::Entity> entities);
		void DeselectAll();

		void SelectAnotherEntity(Gep::Entity entity);

		bool IsEntitySelected(Gep::Entity entity);

	private:
		friend class ImGuiSystem;
		std::unordered_set<Gep::Entity> mSelectedEntities;
		size_t mLastSelectedIndex;
	};
}
