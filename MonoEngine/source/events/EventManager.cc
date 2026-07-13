#include <events/EventManager.hh>
#include <common/Events.h>
#include <utility>

namespace Monoworks 
{
	std::array<std::vector<SCallback>, MW_EVENT_TYPE_COUNT> CEventManager::m_Callbacks;

	CSafeQueue<SEvent> CEventManager::m_EventQueue;

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

			if (type == MW_EVENT_MOUSE_MOVED)
			{
				auto me = event.GetPayload<Events::SMouseMoved>();
				fmt::print("Mouse moved event x: {}, y: {}", me.MouseX, me.MouseY);
			}
		}
	};

}