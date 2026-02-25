/*****************************************************************//**
 * \file   EditorResource.hpp
 * \brief  various functions to allow interacting with the editor. 
 * 
 * \author Travis Gronvold (travis.gronvold@digipen.edu)
 * \date   May 2025
 *********************************************************************/

#pragma once

// help
#include "StringHelp.hpp"
#include "STLHelp.hpp"

// engine
#include "EngineManager.hpp"
#include "Core.hpp"

// gtl
#include "gtl/uuid.hpp"

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

		// creates target 
		template< typename... ComponentRequirements, typename T, typename Getter>
			requires std::is_invocable_r_v<gtl::uuid&, Getter, T&>
		bool DrawEntityDragDropTarget(Gep::EngineManager& em, const std::string& label, std::span<T> components, Getter&& get);



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
        // visible entities in the hierarchy panel. also used for selection order
		std::vector<Gep::Entity> mHierarchyEntities;

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

	template< typename... ComponentRequirements, typename T, typename Getter>
		requires std::is_invocable_r_v<gtl::uuid&, Getter, T&>
	inline bool EditorResource::DrawEntityDragDropTarget(Gep::EngineManager& em, const std::string& label, std::span<T> components, Getter&& get)
	{
        if (components.empty())
            return false; // require at least one component to draw

        const ImVec4 invalidColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        const ImVec4 validColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		const Gep::Signature requirements = em.CreateSignature<ComponentRequirements...>();


		struct InvalidDetails
		{
			Gep::Entity entity = Gep::INVALID_ENTITY;
			std::vector<std::string> missingComponentNames;
            bool didExist = false;
		};

        static std::vector<InvalidDetails> invalidEntitiesDetails; invalidEntitiesDetails.clear();

        // first check all entities for validity
		for (T& c : components)
		{
            const gtl::uuid& uuid = get(c);

			Gep::Entity e = em.FindEntity(uuid);
			if (!em.EntityExists(e))
			{
				auto& back = invalidEntitiesDetails.emplace_back();
                if (uuid.is_valid()) // entity doesnt exist but uuid is valid, so at one point it did exist
                    back.didExist = true;
			}
			else if ((em.GetSignature(e) & requirements) != requirements)
			{
                const Gep::Signature missingComponents = requirements & (~em.GetSignature(e));
                auto& invalidDetails = invalidEntitiesDetails.emplace_back();
                invalidDetails.entity = e;
				em.ForEachComponentBit(missingComponents, [&](const Gep::ComponentData& data)
				{
					invalidDetails.missingComponentNames.push_back(data.name);
				});
			}
		}

		// determine if all components have the same entity, if so display the name of any one of them
		const bool uniform = Gep::IsUniform(components, [&](T& c) -> gtl::uuid& { return get(c); });
		const Gep::Entity first = em.FindEntity(get(components[0]));
        const gtl::uuid& firstUUID = get(components[0]);

		ImGui::BeginGroup();
		ImGui::Text("%s:", label.c_str());
		ImGui::SameLine();

		if (em.EntityExists(first))
		{
			ImGui::TextColored(invalidEntitiesDetails.empty() ? validColor : invalidColor, uniform ? em.GetName(first).c_str() : "-");
		}
		else
		{
            ImGui::TextColored(invalidColor, uniform ? "None" : "-");
		}

        ImGui::EndGroup();
		this->EntityDragDropTarget([&](Gep::Entity e)
		{
			for (T& c : components)
				get(c) = em.GetUUID(e);
		});

        // add a tool tip is there are any invalid entities and return false
		if (invalidEntitiesDetails.size() > 0)
		{
			ImGui::SameLine();
            ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				for (size_t i = 0; i < invalidEntitiesDetails.size(); ++i)
				{
					const InvalidDetails& details = invalidEntitiesDetails[i];
					if (em.EntityExists(details.entity))
					{
						ImGui::Text("Entity:");
						ImGui::SameLine();
                        ImGui::TextColored(validColor, "%s", em.GetName(details.entity).c_str());

						if (!details.missingComponentNames.empty())
						{
							ImGui::Indent();
							for (const std::string& name : details.missingComponentNames)
							{
								ImGui::BulletText("Missing:");
                                ImGui::SameLine();
                                ImGui::TextColored(invalidColor, "%s", name.c_str());
							}
                            ImGui::Unindent();
						}
					}
                    else if (details.didExist)
					{
						ImGui::TextColored(invalidColor, "Entity was deleted");
						ImGui::TextDisabled("Drag a new Entity from the hierarchy here to select");
					}
					else
					{
						ImGui::TextColored(invalidColor, "No entity selected");
						ImGui::TextDisabled("Drag an Entity from the hierarchy here to select");
					}

					if (i < invalidEntitiesDetails.size() - 1)
					{
						ImGui::Spacing();
						ImGui::Separator();
                        ImGui::Spacing();
					}
				}
                ImGui::EndTooltip();
			}

			return false;
		}

		return true;
	}
}
