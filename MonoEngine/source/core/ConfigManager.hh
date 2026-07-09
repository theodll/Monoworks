#pragma once
#include <common/Base.hh>
#include <filesystem>
#include <cstdio>

namespace Monoworks 
{
	// small disposable class
	class CConfigManager 
	{
	public:
		CConfigManager(const std::filesystem::path& path) noexcept;

		void RegisterSection(const std::string& section) noexcept;
		void RegisterValue(const std::string& section, const std::string& key, const std::string& defval) noexcept;
		void Flush() noexcept;
	private:
		std::unordered_map<std::string, std::stringstream> m_Sections;
		std::filesystem::path m_Path;
		bool m_ConfigExists = false;
	};

}



