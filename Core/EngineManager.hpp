/*****************************************************************//**
 * \file   EngineManager.hpp
 * \brief  the core manager of all of the other managers
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <ComponentArray.hpp>
#include <System.hpp>

#include <Application.hpp>

namespace Gep
{
	template <typename T>
	concept TypeHasInitialize = requires(T t)
	{
		{ t.Initialize() } -> std::same_as<void>;
	};

	template <typename T>
	concept TypeHasUpdate = requires(T t, float dt)
	{
		{ t.Update(dt) } -> std::same_as<void>;
	};

	template <typename T, typename Base>
	concept TypeInheritsFrom = std::is_base_of_v<Base, T>;

	template<typename EventType>
	using EventFunction = std::function<void(EventType)>;

	class EngineManager
	{
	public:
		EngineManager()
			: mAvailableEntities()
			, mMarkedEntities()
			, mEntitySignatures()
			, mComponentIDs()
			, mMarkedComponents()
			, mComponentArrays()
			, mNextComponentID(0)
			, mIsRunning(true)
		{
			for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
			{
				mAvailableEntities.push_back(entity);
			}

			mApplication.SetKeyCallback(*this, &EngineManager::SignalEvent<Event::KeyPressed>);
		}

		///////////////////////////////////////////////////////////////////////////////////////////
		// foundational functions /////////////////////////////////////////////////////////////////

		void Start()
		{
			mApplication.Initialize_GLFW();
			mApplication.Initialize_ImGui();
		}

		void End()
		{
			mApplication.End_ImGui();
			mApplication.End_GLFW();
		}

		void FrameStart()
		{
			mApplication.FrameStart_GLFW();
			mApplication.FrameStart_ImGui();
		}

		void FrameEnd()
		{
			mApplication.FrameEnd_ImGui();
			mApplication.FrameEnd_GLFW();
		}

		bool Running() const
		{
			return mApplication.Running();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// entity functions /////////////////////////////////////////////////////////////////////////////

		void SetSignature(Entity entity, Signature signature)
		{
			// Put this entity's signature into the array
			mEntitySignatures[entity] = signature;

			for (auto& [groupSignature, entities] : mEntityGroups)
			{
				// checks if the entities signature is the same as the groups signature, if so add the entity to the group
				if ((signature & groupSignature) == groupSignature)
				{
					entities.insert(entity);
				}
				else
				{
					entities.erase(entity);
				}
			}
		}

		Signature GetSignature(Entity entity) const
		{
			// Put this entity's signature into the array
			return mEntitySignatures.at(entity);
		}

		void MarkEntityForDestruction(Entity entity)
		{
			SignalEvent<Event::EntityDestroyed>({ entity });// calls subscriber functions 

			mMarkedEntities.push_back(entity);
		}

		void DestroyMarkedEntities()
		{
			for (const Entity entity : mMarkedEntities)
			{
				DestroyEntity(entity);
			}

			mMarkedEntities.clear();
		}

		// adds the id back to the id pool
		void DestroyEntity(const Entity entity)
		{
			// destroys each component on an entity if it has one
			for (const auto& [componentID, componentArray] : mComponentArrays)
			{
				if (HasComponent(entity, componentID))
					DestroyComponent(entity, componentID);
			}

			// removes the entity from any systems it might have been in
			for (auto& [groupSignature, entities] : mEntityGroups)
				entities.erase(entity);

			mEntitySignatures[entity].reset();

			mAvailableEntities.push_back(entity);
		}

		Entity CreateEntity()
		{
			Entity id = mAvailableEntities.back();
			mAvailableEntities.pop_back();

			SetSignature(id, 0);

			return id;
		}

		template <typename... ComponentTypes>
		std::unordered_set<Entity>& GetEntities()
		{
			Signature groupSignature;

			// uses folding to create a signature from the arg list
			((groupSignature.set(GetComponentBitPos<ComponentTypes>())), ...);

#ifdef _DEBUG
			if (!mEntityGroups.contains(groupSignature))
			{
				throw std::logic_error("The given group was not registered! You Must First Register The group!");
			}
#endif // _DEBUG
			return mEntityGroups.at(groupSignature);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// component functions //////////////////////////////////////////////////////////////////////////

		template <typename ComponentType>
		void RegisterComponent()
		{
			const std::uint64_t typeID = typeid(ComponentType).hash_code();

			mComponentIDs[typeID] = mNextComponentID;

			mComponentArrays[typeID] = std::make_shared<ComponentArray<ComponentType>>();

			++mNextComponentID;
		}

		template <typename ComponentType>
		void AddComponent(Entity entity, ComponentType component)
		{
			GetComponentArray<ComponentType>()->Insert(entity, component);

			Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
			entitySignature.set(GetComponentBitPos<ComponentType>()); // updates the entities signature with the id of the componet

			// creates an entity group if it doesnt exist
			//mEntityGroups[entitySignature].insert(entity);

			SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly added component
		}

		template<typename ComponentType>
		void MarkComponentForDestruction(Entity entity)
		{
			const uint64_t componentID = typeid(ComponentType).hash_code();

			mMarkedComponents.push_back({ entity, componentID });
		}

		void DestroyMarkedComponents()
		{
			for (const auto& [entity, componentID] : mMarkedComponents)
			{
				DestroyComponent(entity, componentID);
			}

			mMarkedComponents.clear();
		}

		void DestroyComponent(Entity entity, uint64_t component)
		{
			GetComponentArray(component)->Erase(entity);

			Signature entitySignature = GetSignature(entity); // gets the existing signature of the entity
			entitySignature.set(mComponentIDs.at(component), false); // removes the components id from the entities signature

			SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly removed component
		}

		template<typename ComponentType>
		ComponentType& GetComponent(Entity entity)
		{
			return GetComponentArray<ComponentType>()->GetComponent(entity);
		}

		template <typename ComponentType>
		bool HasComponent(const Entity entity) const
		{
			// if it has the transform signature
			Signature componentSig;
			componentSig.set(GetComponentBitPos<ComponentType>());
			return ((GetSignature(entity) & componentSig) == componentSig);
		}

		bool HasComponent(const Entity entity, const uint64_t componentID) const
		{
			Signature componentSig;
			componentSig.set(mComponentIDs.at(componentID));
			return ((GetSignature(entity) & componentSig) == componentSig);
		}

		template<typename ComponentType>
		ComponentBitPos GetComponentBitPos() const
		{
			const std::uint64_t typeID = typeid(ComponentType).hash_code();

			return mComponentIDs.at(typeID);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// system functions /////////////////////////////////////////////////////////////////////////////

		template <typename SystemType>
		void RegisterSystem()
		{
			static_assert(TypeInheritsFrom<SystemType, ISystem>, "SystemType must inherit from ISystem");

			const uint64_t typeID = typeid(SystemType).hash_code();

			mSystems[typeID] = std::make_shared<SystemType>(*this);
		}

		template <typename SystemType>
		void SetSystemSignature(Signature signature)
		{
			const uint64_t typeID = typeid(SystemType).hash_code();

			mSystemSignatures[typeID] = signature;
		}

		template <typename... ComponentTypes>
		void RegisterGroup()
		{
			Signature groupSignature;

			// uses folding to create a signature from the arg list
			((groupSignature.set(GetComponentBitPos<ComponentTypes>())), ...);

			mEntityGroups[groupSignature];
		}

		// initializes all systems in the order registered
		template <typename SystemType>
		void Initialize()
		{
			static_assert(TypeHasInitialize<SystemType>, "You passed a type to Initialize that has no Initialize function!");

			std::static_pointer_cast<SystemType>(mSystems.at(typeid(SystemType).hash_code()))->Initialize();
		}

		// updates all systems in the order registered
		template<typename SystemType>
		void Update(float dt)
		{
			static_assert(TypeHasUpdate<SystemType>, "Attempting to update a class with no update!");

			std::static_pointer_cast<SystemType>(mSystems.at(typeid(SystemType).hash_code()))->Update(dt);
		}

		template<typename SystemType>
		void RenderImGui(float dt)
		{
			static_assert(TypeHasUpdate<SystemType>, "Attempting to update a class with no update!");

			std::static_pointer_cast<SystemType>(mSystems.at(typeid(SystemType).hash_code()))->RenderImGui(dt);
		}


		/////////////////////////////////////////////////////////////////////////////////////////////////
		// event functions //////////////////////////////////////////////////////////////////////////////

		template<typename SystemType, typename EventType, typename MemberFunctionPtr>
		void SubscribeToEvent(MemberFunctionPtr function)
		{
			const uint64_t typeID = typeid(SystemType).hash_code();

			SystemType& system = *std::static_pointer_cast<SystemType>(mSystems.at(typeID)); // call member function

			GetEventFunctions<EventType>().emplace_back(std::bind(function, std::ref(system), std::placeholders::_1));
		}

		template <typename EventType>
		void SignalEvent(const EventType& eventData)
		{
			GetEventData<EventType>().push_back(eventData);
		}

		template <typename EventType>
		void StartEvent()
		{
			// the order of these for loops is preference
			for (const EventType& eventData : GetEventData<EventType>())
			{
				for (EventFunction<EventType>& eventFunction : GetEventFunctions<EventType>())
				{
					eventFunction(eventData);
				}
			}
			GetEventData<EventType>().clear();
		}

	private:
		template <typename ComponentType>
		std::shared_ptr<ComponentArray<ComponentType>> GetComponentArray()
		{
			const std::uint64_t typeID = typeid(ComponentType).hash_code();

			return std::static_pointer_cast<ComponentArray<ComponentType>>(mComponentArrays.at(typeID));
		}

		std::shared_ptr<IComponentArray> GetComponentArray(uint64_t typeID)
		{
			return mComponentArrays.at(typeID);
		}

		// keeps a lists of subscribers for each type of event
		template<typename EventType>
		std::vector<EventFunction<EventType>>& GetEventFunctions()
		{
			static std::vector<EventFunction<EventType>> subscribers;
			return subscribers;
		}

		// stores the event data for each event
		template<typename EventType>
		std::vector<EventType>& GetEventData()
		{
			static std::vector<EventType> eventData;
			return eventData;
		}

	private:
		// entities
		std::vector<Entity> mAvailableEntities; // list of unused entity ids
		std::vector<Entity> mMarkedEntities; // entities that are marked to be destroyed
		std::unordered_map<Entity, Signature> mEntitySignatures; // this keeps track of which components an entity has
		std::unordered_map<Signature, std::unordered_set<Entity>> mEntityGroups; // used by systems, holds all entities with a matching components

		// components
		ComponentBitPos mNextComponentID; // used for assigning bits in an entities signature
		std::unordered_map<uint64_t, ComponentBitPos> mComponentIDs; // maps the type id to the component
		std::vector<std::pair<Entity, uint64_t>> mMarkedComponents;   // The entity and the Entities component type ids.
		std::unordered_map<uint64_t, std::shared_ptr<IComponentArray>> mComponentArrays; // maps the component typeid to an array of the component

		// systems
		std::unordered_map<uint64_t, Signature> mSystemSignatures; // the signatures of all of the systems maps the typeid of a system to its signature
		std::unordered_map<uint64_t, std::shared_ptr<ISystem>> mSystems;// maps the typeid of a system to the actual system class


		Application mApplication;

		bool mIsRunning;
	};
}
