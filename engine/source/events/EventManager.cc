#include <events/EventManager.hh>
#include <common/Events.hh>

#include <utility>
#include <functional>

namespace Monoworks 
{
	std::array<std::vector<SCallback>, MW_EVENT_TYPE_COUNT> CEventManager::m_Callbacks;

	CSafeQueue<SEvent> CEventManager::m_EventQueue;

	void CEventManager::Init() noexcept 
	{
		MW_PROFILE_FUNC;;
		MW_INFO("Initialize CEventManager");
	};

	void CEventManager::Shutdown() noexcept 
	{
		MW_PROFILE_FUNC;;
		MW_INFO("Shutdown CEventManager");
	};

	size_t CEventManager::Subscribe(EEventType type, std::function<bool(SEvent&)> func)
	{
		MW_PROFILE_FUNC;
		size_t id = m_Callbacks.size();
		MW_TRACE("Subscribed listener {}({}) to event {}", id, reinterpret_cast<void*>(func.target<bool(*)(SEvent&)>()), EventTypeToString(type));
		m_Callbacks[( u8 )type].emplace_back( 1, std::move( func ) );
		return id;
	};

	void CEventManager::Unsubscribe( EEventType type, size_t id )
	{
		MW_PROFILE_FUNC;
		MW_TRACE( "Delete listener {} from event {}", id, EventTypeToString( type ) );

		auto& vec = m_Callbacks[( u8 )type];
		vec[id].Alive = false;
		vec[id].Function = nullptr;
	}

	void CEventManager::ProcessEvents() noexcept
	{
		MW_PROFILE_FUNC;;
		while (m_EventQueue.GetSize() > 0)
		{
			auto event = m_EventQueue.Front();
			u8 type = (u8)event.GetType();

			for (auto& callb : m_Callbacks[type])
			{
				if (!callb.Alive)
					return;

				auto func = callb.Function;
				
				auto res = std::invoke(func, event);

				if (res)
				{
					event.SetHandled(res);
					break; 
				}
			}
		}
	};

}