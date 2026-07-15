#pragma once
#include <common/Base.hh>
#include <filesystem>
#include <cstdio>
#include <concepts>

extern "C" {
	#include <dictionary.h>
	#include <iniparser.h>
}

namespace Monoworks 
{
	template <typename T>
	concept IniGettable =
		std::integral<T> || std::floating_point<T> || std::same_as<std::decay_t<T>, std::string> || std::same_as<std::decay_t<T>, bool>;

	// small disposable class
	class CConfigManager 
	{
	public:
		CConfigManager(const char* path) noexcept;
		virtual ~CConfigManager() noexcept;
	


		void RegisterSection(const std::string& section) noexcept;
		void RegisterValue(const std::string& section, const std::string& key, const std::string& defval) noexcept;
		void Flush() noexcept;
		
		template <typename T = std::string> requires IniGettable<T>
		[[nodiscard]] T Get(const std::string& section, const std::string& key) noexcept
		{
			MW_PROFILE_FUNC();
			if (!m_Inifile)
				m_Inifile = iniparser_load(m_CPath);

			T temp;

			auto keyValS = std::format("{}:{}", section, key);
			const char* keyVal = keyValS.c_str();

			if constexpr (std::same_as<std::decay_t<T>, bool>)
			{
				temp = static_cast<bool>(iniparser_getboolean(m_Inifile, keyVal, 0));
			}
			else if constexpr (std::same_as<std::decay_t<T>, std::string>)
			{
				const char* res = iniparser_getstring(m_Inifile, keyVal, "Not Found");
				temp = res ? res : "";

				if (temp == "Not Found")
				{
					MW_WARN("Unable to get string {}: Not exsistant", keyVal);
					return T();
				}
			}
			else if constexpr (std::integral<T>)
			{
				int temptemp = iniparser_getint(m_Inifile, keyVal, -1);

				if (temptemp == -1)
				{
					MW_WARN("Unable to get integer {}: Not exsistant", keyVal);
					return 0;
				}

				temp = static_cast<T>(temptemp);
			}
			else if constexpr (std::floating_point<T>)
			{
				temp = iniparser_getdouble(m_Inifile, keyVal, -1.0f);

				if (temp == -1.0f)
				{
					MW_WARN("Unable to get double {}: Not exsistant", keyVal);
				}
			}
			return temp;
		}

	private:


		std::unordered_map<std::string, std::stringstream> m_Sections;
		const char* m_CPath;
		std::filesystem::path m_Path;
		dictionary* m_Inifile = nullptr;
		bool m_ConfigExists = false;


	};
}



