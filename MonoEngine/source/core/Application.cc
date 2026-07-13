#include "Application.hh"
#include "CVarManager.hh"
#include "ConfigManager.hh"
#include <events/EventManager.hh>
#include <events/Event.hh>

#include <functional>

namespace Monoworks
{
	CApplication* CApplication::m_Singleton;

	void CApplication::Init(const SApplicationCreateInfos* pInfos) noexcept
	{
		CCvarManager::Init();
		CLogManager::Init();
		CMemoryManager::Init();
		CEventManager::Init();
		
		auto callb = [](SEvent&) 
			{
				fmt::print("click event"); 
				return false; 
			};

		CEventManager::Subscribe(MW_EVENT_TYPE_MOUSE_CLICKED, callb);

	}

	void CApplication::Shutdown() noexcept
	{
		CEventManager::Shutdown();
		CMemoryManager::Shutdown();
		CLogManager::Shutdown();
		CCvarManager::Shutdown();
	}

	void CApplication::Frame()
	{
		// called once per frame

		CEventManager::ProcessEvents();
	}
}