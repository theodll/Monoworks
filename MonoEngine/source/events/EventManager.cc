#include <events/EventManager.hh>

namespace Monoworks 
{
	std::array<std::vector<std::function<bool(SEvent&)>>, MW_EVENT_TYPE_COUNT> CEventManager::m_Callbacks;

	CSafeQueue<SEvent> CEventManager::m_EventQueue;

	void CEventManager::Init() noexcept 
	{

	};

	void CEventManager::Shutdown() noexcept 
	{
	};

	void CEventManager::Subscribe(EEventType type, std::function<bool(SEvent&)>& func)
	{
		m_Callbacks[(u8)type].emplace_back( 1, std::move(func));
	};

	void CEventManager::EnlistEvent(SEvent& event) noexcept
	{
		event.SetHandled(false);
		m_EventQueue.Push(std::move(event));
	};

	void CEventManager::ProcessEvents() noexcept
	{
		while (m_EventQueue.GetSize() != 0)
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
					goto FINISH_EVENT; 
				}
			}

		FINISH_EVENT:

		}
	};

}