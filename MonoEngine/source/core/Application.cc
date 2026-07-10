#include "Application.hh"
#include "CVarManager.hh"
#include "ConfigManager.hh"

namespace Monoworks
{
	CApplication* CApplication::m_Singleton;

	void CApplication::Init(const SApplicationCreateInfos* pInfos) noexcept
	{
		CCvarManager::Init();
		CLogManager::Init();
		CMemoryManager::Init();
		
	


	}

	void CApplication::Shutdown() noexcept
	{
		CMemoryManager::Shutdown();
		CLogManager::Shutdown();
		CCvarManager::Shutdown();
	}

	void CApplication::Frame()
	{
	
	}
}