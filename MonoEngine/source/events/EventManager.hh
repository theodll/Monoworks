#pragma once
#include <common/Base.hh>
#include <common/SafeQueue.hh>

#include "Event.hh"

#include <functional>
#include <array>
#include <vector>

namespace Monoworks 
{
	/**
	 * @brief Callback struct to house any data related to event callbacks.
	 */
	struct SCallback 
	{
		u8 Alive;
		/**
		 * @brief Callback function of the callback.
		 */
		std::function<bool(SEvent&)> Function;
	};

	inline const char* EventTypeToString(EEventType type)
	{
		switch (type)
		{
		case MW_EVENT_WINDOW_CLOSE:          return "MW_EVENT_WINDOW_CLOSE";
		case MW_EVENT_WINDOW_FOCUS:          return "MW_EVENT_WINDOW_FOCUS";
		case MW_EVENT_WINDOW_LOST_FOCUS:     return "MW_EVENT_WINDOW_LOST_FOCUS";
		case MW_EVENT_WINDOW_MINIMIZE:       return "MW_EVENT_WINDOW_MINIMIZE";
		case MW_EVENT_WINDOW_RESIZE:         return "MW_EVENT_WINDOW_RESIZE";

		case MW_EVENT_APP_TICK:              return "MW_EVENT_APP_TICK";
		case MW_EVENT_APP_UPDATE:            return "MW_EVENT_APP_UPDATE";
		case MW_EVENT_APP_RENDER:            return "MW_EVENT_APP_RENDER";

		case MW_EVENT_KEY_PRESSED:           return "MW_EVENT_KEY_PRESSED";
		case MW_EVENT_KEY_RELEASED:          return "MW_EVENT_KEY_RELEASED";
		case MW_EVENT_KEY_TYPED:             return "MW_EVENT_KEY_TYPED";

		case MW_EVENT_MOUSE_BUTTON_PRESSED:  return "MW_EVENT_MOUSE_BUTTON_PRESSED";
		case MW_EVENT_MOUSE_BUTTON_RELEASED: return "MW_EVENT_MOUSE_BUTTON_RELEASED";
		case MW_EVENT_MOUSE_MOVED:           return "MW_EVENT_MOUSE_MOVED";
		case MW_EVENT_MOUSE_SCROLLED:        return "MW_EVENT_MOUSE_SCROLLED";

		case MW_EVENT_TYPE_COUNT:            return "MW_EVENT_TYPE_COUNT";
		}

		return "Unknown Event";
	}


	/**
	* @brief Static class to manage Events
	*/
	class CEventManager 
	{
	public:
		/**
		 * @brief Initializes the Event Manager.
		 */
		static void Init() noexcept;

		/**
		 * @brief Shuts the Event Manager down.
		 */
		static void Shutdown() noexcept;

		/**
		 * @brief Function to subscribe a listener to an event.
		 * Subscribe to the actions of one certain event type.
		 * 
		 * @param type Type of the Event to subscribe
		 * @param func Function Pointer to the function that should be executed when this event is processed
		 */
		static void Subscribe(EEventType type, std::function<bool(SEvent&)> func);

		/**
		 * @brief Thread-Safe way to enlist a generic event into the event queue.
		 * 
		 * @param event Event you want to enlist.
		 */
		template <typename T> requires impl::ValidEvent<T>
		static void EmitEvent(T& e, EEventType type) noexcept
		{
			SEvent event{};
			event.SetType(type);
			event.SetHandled(false);
			event.SetPayload<T>(e);

			m_EventQueue.Push(std::move(event));
		}

		template <typename T> requires impl::ValidEvent<T>
		static void EmitEventNonDeffered(T& e, EEventType type) noexcept
		{
			SEvent event{};
			event.SetType(type);
			event.SetHandled(false);
			event.SetPayload<T>(e);

			for (auto& callb : m_Callbacks[type])
			{
				if (!callb.Alive)
					return;

				auto func = callb.Function;

				auto res = func(event);
				if (res)
				{
					event.SetHandled(res);
					break;
				}
			}
		}

		/**
		 * @brief Process all the events in the event queue.
		 * Must be called before beginning to process any frame to avoid conflicts!
		 */
		static void ProcessEvents() noexcept;

	private:
		/**
		 * @brief Two dimensional array of all registered callbacks.
		 * 
		 * The index to index into the array holding the vector of the callbacks is equivalent to the type of any event (eg. MW_EVENT_TYPE_MOUSE_CLICKED) and therefore has a size of MW_EVENT_TYPE_COUNT.
		 * The inner array contains all registered callbacks corresponding to the event type in an arbitrary order. 
		 */
		static std::array<std::vector<SCallback>, MW_EVENT_TYPE_COUNT> m_Callbacks;


		/**
		 * @brief Thread-Safe blocking Event Queue of generic Events.
		 */
		static CSafeQueue<SEvent> m_EventQueue;

	};


}
