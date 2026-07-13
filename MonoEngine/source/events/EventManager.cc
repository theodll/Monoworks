#include <events/EventManager.hh>
#include <utility>

namespace Monoworks 
{
	std::array<std::vector<SCallback>, MW_EVENT_TYPE_COUNT> CEventManager::m_Callbacks;

	CSafeQueue<SEvent> CEventManager::m_EventQueue;

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

	void CEventManager::Init() noexcept 
	{
		MW_INFO("Initialize CEventManager");
	};

	void CEventManager::Shutdown() noexcept 
	{
		MW_INFO("Shutdown CEventManager");
	};

	void CEventManager::Subscribe(EEventType type, std::function<bool(SEvent&)> func)
	{
		MW_TRACE("Subscribed listener {} to event {}", reinterpret_cast<void*>(func.target<bool(*)(SEvent&)>()), EventTypeToString(type));
		m_Callbacks[(u8)type].emplace_back( 1, std::move(func));
	};

	void CEventManager::ProcessEvents() noexcept
	{
		while (m_EventQueue.GetSize() > 0)
		{
			auto event = m_EventQueue.Front();
			u8 type = (u8)event.GetType();

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
	};

}