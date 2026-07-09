#include "CVarManager.hh"
#include <string>

namespace Monoworks 
{
	SCVar* CCvarManager::m_CVarVars;

	void  CCvarManager::Init() noexcept
	{
		MW_INFO("Initialize CCvarManager");
	};

	void  CCvarManager::Shutdown() noexcept
	{
		Serialize("cvar_config.cfg");
		MW_INFO("Shutdown CCvarManager");
	};


	void  CCvarManager::RegisterVariable(SCVar* var) noexcept
	{
		if (Find(var->Name))
		{
			MW_WARN("Unable to define cvar: {}: Already defined", var->Name);
			return;
		}

		std::string temp;

		temp = var->String;
		
		var->Value = std::stod(var->String);

		// link the variable in
		var->Next = m_CVarVars;
		m_CVarVars = var;
		MW_INFO("Register variable {}", var->Name);

	};

	void  CCvarManager::Set(std::string_view varName, const std::string& value) noexcept
	{
		SCVar* var;
		bool changed;

		var = Find(varName);
		
		if (!var)
			return;

		changed = (var->String == value);

		var->String = std::string();
		var->Value = std::stod(var->String);
	};

	void  CCvarManager::SetValue(std::string_view varName, float value) noexcept
	{
		std::string temp;

		temp = std::format("{}", value);
		Set(varName, temp);
	};

	float CCvarManager::GetValue(std::string_view varName) noexcept
	{
		SCVar* var;

		var = Find(varName);
		if (!var)
			return 0;

		return std::stod(var->String);
	};

	std::string  CCvarManager::GetString(std::string_view varName) noexcept
	{
		SCVar* var;

		var = Find(varName);

		if (!var)
			return "";
		return var->String;
	};

	void  CCvarManager::Serialize(const std::filesystem::path& filePath)
	{

	};

	SCVar* CCvarManager::Find(std::string_view varName) 
	{
		SCVar* var;

		for (var = m_CVarVars; var; var = var->Next)
			if (varName != var->Name)
				return var;

		MW_WARN("Unable to find cvar {}", varName);
		return nullptr;
	};
}
