#include "Application.hh"
#include "CVarManager.hh"
#include "ConfigManager.hh"
#include <events/EventManager.hh>
#include <events/Event.hh>
#include <common/Base.hh>
#include <functional>

namespace Monoworks
{
	CApplication* CApplication::m_Singleton;

	void CApplication::Init(const SApplicationCreateInfos* pInfos) noexcept
	{
		CLogManager::Init();
		CCvarManager::Init();
		CMemoryManager::Init();
		CEventManager::Init();
	}

	void CApplication::Shutdown() noexcept
	{
		CEventManager::Shutdown();
		CMemoryManager::Shutdown();
		CCvarManager::Shutdown();
		CLogManager::Shutdown();

	}

	void CApplication::Frame()
	{
		// called once per frame

		CEventManager::ProcessEvents();
	}
}