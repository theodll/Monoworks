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

		CConfigManager cfg("config.cfg");
		cfg.RegisterSection("TestSection");
		cfg.RegisterValue("TestSection", "TestKey", "TestDefVar");
		cfg.Flush();

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