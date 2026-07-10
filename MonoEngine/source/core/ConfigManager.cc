#include "ConfigManager.hh"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <type_traits>

namespace Monoworks 
{


	CConfigManager::CConfigManager(const std::filesystem::path& path) noexcept : m_Path(path)
	{
		m_ConfigExists = std::filesystem::exists(path);

		if (!m_ConfigExists)
			return;

		const char* cstr = path.string().c_str();

		m_Inifile = iniparser_load(cstr);

		iniparser_dump(m_Inifile, stderr); 
	}

	CConfigManager::~CConfigManager() noexcept
	{
		iniparser_freedict(m_Inifile);
	}

	void CConfigManager::RegisterSection(const std::string& section) noexcept
	{
		if (m_ConfigExists)
			return;

		std::stringstream ss;

		ss << '[' << section << ']' << '\n';

		if (m_Sections.find(section) != m_Sections.end())
		{
			MW_WARN("Unable to register section {}: Already exists", section);
			return;
		}

		m_Sections[section] = std::move(ss);
		MW_INFO("Register section {} in config {}", section, m_Path.string());
	}


	void CConfigManager::RegisterValue(const std::string& section, const std::string& key, const std::string& defval) noexcept
	{
		if (m_ConfigExists)
			return;

		if (!m_Sections.contains(section))
		{
			MW_WARN("Unable to register key {} in section {}: Section doesn't exist", key, section);
			return;
		}

		std::stringstream& ss = m_Sections[section];

		std::string temp = ss.str();
		
		if (temp.find(key) != std::string::npos)
		{
			MW_WARN("Unable to register key {} in section {}: Already exists", key, section);
			return;
		}

		ss << '\x09' << key << '=' << defval << '\n';
	}


	void CConfigManager::Flush() noexcept
	{
		if (m_ConfigExists)
			return;

		std::ofstream cfgFile(m_Path);
		
		for (auto& it : m_Sections)
		{
			cfgFile << it.second.str();
		}
		
		cfgFile.close();
		MW_INFO("Flush config file to disk {}", m_Path.string());
	}


	
}
