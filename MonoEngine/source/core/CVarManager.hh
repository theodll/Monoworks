#pragma once
#include <string>
#include <string_view>
#include <filesystem>

namespace Monoworks 
{
	struct SCVar 
	{
		std::string		Name;
		std::string		String;
		bool			Archive;		
		bool			Server;		
		float			Value;
		SCVar*			Next;
	};

	class CCvarManager 
	{
	public:
		static void Init() noexcept;
		static void Shutdown() noexcept;

		static void RegisterVariable(SCVar* var) noexcept;
		static void Set(std::string_view varName, const std::string& value) noexcept;
		static void SetValue(std::string_view varName, float value) noexcept;
		[[nodiscard]] static float GetValue(std::string_view varName) noexcept;
		[[nodiscard]] static std::string GetString(std::string_view varName) noexcept;
	private:
		[[nodiscard]] static SCVar* Find(std::string_view varName);
		static void Serialize(const std::filesystem::path& filePath); 

		static SCVar* m_CVarVars;
	};
}
