/*****************************************************************//**
 * \file   EngineManager.hpp
 * \brief  the core manager of all of the other managers
 * 
 * \author 2018t
 * \date   July 2024
 *********************************************************************/

#pragma once

#include <Core.hpp>

#include <EntityManager.hpp>
#include <SystemManager.hpp>
#include <ComponentManager.hpp>
#include <EventManager.hpp>

namespace Gep
{
	class EngineManager
	{
	public:
		EngineManager()
			: mComponentManager(std::make_unique<ComponentManager>())
			, mEntityManager(std::make_unique<EntityManager>())
			, mSystemManager(std::make_unique<SystemManager>(*this))
			, mEventManager(std::make_unique<EventManager>())
			, mIsRunning(true)
		{}

		void ExitEngine()
		{
			mIsRunning = false;
		}

		bool IsRunning() const
		{
			return mIsRunning;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// entity functions /////////////////////////////////////////////////////////////////////////////

		Entity CreateEntity()
		{
			return mEntityManager->CreateEntity();
		}

		void DestroyEntity(Entity entity)
		{
			mSystemManager->Event_EntityDestroyed(entity);

			mEntityManager->DestroyEntity(entity);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// component functions //////////////////////////////////////////////////////////////////////////

		template <typename ComponentType>
		void RegisterComponent()
		{
			mComponentManager->Register<ComponentType>();
		}

		template <typename ComponentType>
		void AddComponent(Entity entity, ComponentType component)
		{
			mComponentManager->AddComponent(entity, component);

			Signature entitySignature = mEntityManager->GetSignature(entity); // gets the existing signature of the entity
			ComponentID componentID = mComponentManager->GetComponentID<ComponentType>(); // gets the component id of the component being added
			
			entitySignature.set(componentID); // updates the entities signature with the id of the componet

			mEntityManager->SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly added component

			mSystemManager->Event_EntitySignatureChanged(entity, entitySignature);
		}

		template <typename ComponentType>
		void RemoveComponent(Entity entity)
		{
			mComponentManager->RemoveComponent<ComponentType>(entity);

			Signature entitySignature = mEntityManager->GetSignature(entity); // gets the existing signature of the entity
			ComponentID componentID = mComponentManager->GetComponentID<ComponentType>(); // gets the component id of the component being removed

			entitySignature.set(componentID, false); // removes the components id from the entities signature

			mEntityManager->SetSignature(entity, entitySignature); // sets the signature of the entity to the signature with the newly removed component

			mSystemManager->Event_EntitySignatureChanged(entity, entitySignature);
		}

		template<typename ComponentType>
		ComponentType& GetComponent(Entity entity)
		{
			return mComponentManager->GetComponent<ComponentType>(entity);
		}

		// use sparingly
		Signature GetSignature(Entity entity) const
		{
			return mEntityManager->GetSignature(entity);
		}

		template <typename ComponentType>
		bool HasComponent(Entity entity) const
		{
			// if it has the transform signature
			Signature transformSignature;
			transformSignature.set(mComponentManager->GetComponentID<ComponentType>());
			return ((mEntityManager->GetSignature(entity) & transformSignature) == transformSignature);
		}

		template<typename ComponentType>
		ComponentID GetComponentID()
		{
			return mComponentManager->GetComponentID<ComponentType>();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// system functions /////////////////////////////////////////////////////////////////////////////

		template<typename SystemType>
		void RegisterSystem()
		{
			mSystemManager->RegisterSystem<SystemType>();
		}

		template<typename SystemType>
		void SetSystemSignature(Signature signature)
		{
			mSystemManager->SetSignature<SystemType>(signature);
		}

		// initializes all systems in the order registered
		void Init()
		{
			mSystemManager->Init();
		}

		// updates all systems in the order registered
		void Update(float dt)
		{
			mSystemManager->Update(dt);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// event functions //////////////////////////////////////////////////////////////////////////////

		template<typename EventType, typename SystemType, typename MemberFunction>
		void SubscribeToEvent(SystemType& system, const MemberFunction& mf)
		{
			mEventManager->SubscribeToEvent<EventType>(system, mf);
		}

		template <typename EventType>
		void SignalEvent(const EventType& eventData)
		{
			mEventManager->SignalEvent<EventType>(eventData);
		}

		template <typename EventType>
		void StartEvent()
		{
			mEventManager->StartEvent<EventType>();
		}

	private:
		std::unique_ptr<ComponentManager> mComponentManager;
		std::unique_ptr<EntityManager> mEntityManager;
		std::unique_ptr<SystemManager> mSystemManager;
		std::unique_ptr<EventManager> mEventManager;

		bool mIsRunning;
	};
}
