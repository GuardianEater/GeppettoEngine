/*****************************************************************//**
 * \file   EditorResource.hpp
 * \brief  various functions to allow interacting with the editor. 
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

#include "Core.hpp"
#include "StringHelp.hpp"

namespace Client
{
	class EditorResource
	{
	public:
		void SmartSelectEntity(Gep::Entity entity, GLFWwindow* window = nullptr);

		void SelectEntity(Gep::Entity entity);
		void DeselectEntity(Gep::Entity entity);

		void SelectAll(std::span<const Gep::Entity> entities);
		void DeselectAll();

		void SelectAnotherEntity(Gep::Entity entity);

		bool IsEntitySelected(Gep::Entity entity) const;

		const std::unordered_set<Gep::Entity>& GetSelectedEntities() const;

		// will only accept imgui drag drop targets from the asset browser with the given extension. Calls the user function with the dropped path
		template <typename Func>
			requires std::invocable<Func, const std::filesystem::path&>
		void AssetBrowserDropTarget(const std::vector<std::string>& allowedExtension, Func&& onDrop) const;

		// calls the given function for each entity dropped
		template <typename FunctionType, typename ExitFunctionType>
			requires std::invocable<FunctionType, Gep::Entity>
		void EntitiesDragDropTarget(FunctionType func, ExitFunctionType exitFunc = [](){});

		// calls the given function for the entity dropped (accepts many entities however will only use first entity)
		template <typename FunctionType>
			requires std::invocable<FunctionType, Gep::Entity>
		void EntityDragDropTarget(FunctionType func);

		template <typename Func>
		requires std::invocable<Func, const std::filesystem::path&>
		void AssetBrowserDropTarget(Func&& onDrop) const;

		void LabledInput_Float(const std::string& label, float* v, float columnWidth = 100.0f, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		void LabledInput_Float3(const std::string& label, float* v, float columnWidth = 100.0f, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		void LabledInput_Float4(const std::string& label, float* v, float columnWidth = 100.0f, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);


	private:
		void LabledInput_Setup(const std::string& label, float columnWidth);
		void LabledInput_End();

	private:
		std::vector<Gep::Entity> mHierarchyEntities; // the order of the entities in the hierarchy

		friend class ImGuiSystem;
		std::unordered_set<Gep::Entity> mSelectedEntities;
		size_t mLastSelectedIndex;
	};

	template<typename Func>
	requires std::invocable<Func, const std::filesystem::path&>
	inline void EditorResource::AssetBrowserDropTarget(const std::vector<std::string>& allowedExtensions, Func&& onDrop) const
	{
		std::string payloadkey = "ASSET_BROWSER";

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadkey.c_str()))
			{
				IM_ASSERT(payload->DataSize == sizeof(char) * (strlen((const char*)payload->Data) + 1));
				const char* path = (const char*)payload->Data;

				// get the dropped filepath and get the lowercase extension out of it
				std::filesystem::path droppedPath(path);
				std::string droppedExtension = droppedPath.extension().string();
				Gep::ToLower(droppedExtension);

				// convert the allowed extensions to lowercase
				std::vector<std::string> lowercaseExtensions = allowedExtensions;
				for (std::string& str : lowercaseExtensions)
					Gep::ToLower(str);

				// was the dropped path an extension of interest
				auto it = std::find(lowercaseExtensions.begin(), lowercaseExtensions.end(), droppedExtension);

				if (it != lowercaseExtensions.end())
				{
					onDrop(droppedPath);
				}
				else
				{
					Gep::Log::Warning("The drag drop destination only accepts files of type: ", lowercaseExtensions, " The given extension was: ", droppedPath.extension());
				}

			}

			ImGui::EndDragDropTarget();
		}
	}

	template<typename Func>
		requires std::invocable<Func, const std::filesystem::path&>
	inline void EditorResource::AssetBrowserDropTarget(Func&& onDrop) const
	{
		std::string payloadkey = "ASSET_BROWSER";

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadkey.c_str()))
			{
				IM_ASSERT(payload->DataSize == sizeof(char) * (strlen((const char*)payload->Data) + 1));
				const char* path = (const char*)payload->Data;

				// get the dropped filepath and get the lowercase extension out of it
				std::filesystem::path droppedPath(path);

				onDrop(droppedPath);
			}

			ImGui::EndDragDropTarget();
		}
	}

	template <typename FunctionType, typename ExitFunctionType>
		requires std::invocable<FunctionType, Gep::Entity>
	void EditorResource::EntitiesDragDropTarget(FunctionType func, ExitFunctionType funcExit)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
			{
				Gep::Entity* droppedEntities = (Gep::Entity*)payload->Data;
				size_t droppedEntityCount = payload->DataSize / sizeof(Gep::Entity);
				std::set<Gep::Entity> droppedEntitiesSet(droppedEntities, droppedEntities + droppedEntityCount);

				for (Gep::Entity droppedEntity : droppedEntitiesSet)
				{
					func(droppedEntity);
				}
				funcExit();
			}
			ImGui::EndDragDropTarget();
		}
	}

	template <typename FunctionType>
		requires std::invocable<FunctionType, Gep::Entity>
	void EditorResource::EntityDragDropTarget(FunctionType func)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
			{
				Gep::Entity* droppedEntities = (Gep::Entity*)payload->Data;
				size_t droppedEntityCount = payload->DataSize / sizeof(Gep::Entity);
				std::set<Gep::Entity> droppedEntitiesSet(droppedEntities, droppedEntities + droppedEntityCount);

				if (!droppedEntitiesSet.empty())
				{
					func(*droppedEntitiesSet.begin());
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
}
