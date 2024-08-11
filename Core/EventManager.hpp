/*****************************************************************//**
 * \file   EventManager.hpp
 * \brief  manages all events that are 
 * 
 * \author 2018t
 * \date   August 2024
 *********************************************************************/

#pragma once

// core
#include <Core.hpp>

#include <BoundFunction.hpp>

namespace Gep
{
	// base class for all events
	struct IEvent
	{
		virtual ~IEvent() = default;
	};

	template<typename EventType>
	using EventFunction = std::function<void(EventType)>;

	class EventManager
	{
	public:

		template<typename EventType, typename SystemType, typename MemberFunction>
		void SubscribeToEvent(SystemType& system, const MemberFunction& mf)
		{
			GetEventFunctions<EventType>().emplace_back(std::bind(mf, system, std::placeholders::_1));
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

	};
}
