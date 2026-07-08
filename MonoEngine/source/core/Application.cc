#include "Application.hh"


namespace Monoworks
{
	CApplication* CApplication::m_Singleton;

	void CApplication::Init(const SApplicationCreateInfos* pInfos) noexcept
	{
		CLogManager::Init();
		CMemoryManager::Init();
	}

	void CApplication::Shutdown() noexcept
	{
		CMemoryManager::Shutdown();
		// CLogManager::Shutdown();
	}

	void CApplication::Frame()
	{
		
	}
}