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
		cfg.RegisterSection("Section 2"); 
		cfg.RegisterValue("Section 2", "Key 2", "Value 2");
		cfg.RegisterValue("Section 2", "Key 3", "Value 2");	
		cfg.Flush();

		std::string test = cfg.Get<std::string>("TestSection", "TestKey");

		MW_INFO("{}", test);

		
	


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