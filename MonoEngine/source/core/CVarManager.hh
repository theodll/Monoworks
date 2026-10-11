#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Monoworks
{
	struct SCVar
	{
		SCVar() = default;
		~SCVar();

		SCVar( const SCVar& ) = delete;
		SCVar& operator=( const SCVar& ) = delete;
		SCVar( SCVar&& ) = delete;
		SCVar& operator=( SCVar&& ) = delete;

		std::string	Name;
		std::string	String;
		bool		Archive = false;
		bool		Server  = false;
		float		Value   = 0.0f;
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
		friend struct SCVar;

		static void Unregister(SCVar* var) noexcept;
		[[nodiscard]] static SCVar* Find(std::string_view varName);
		static void Serialize(const std::filesystem::path& filePath);
	};
}