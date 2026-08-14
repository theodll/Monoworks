#ifndef DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#endif


#include <doctest/doctest.h>

#include <core/ConfigManager.hh>

#include <chrono>
#include <filesystem>
#include <string>

namespace
{
	std::filesystem::path MakeTempConfigPath(std::string_view testName)
	{
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		return std::filesystem::temp_directory_path() /
			std::filesystem::path("monoworks_" + std::string(testName) + "_" + std::to_string(stamp) + ".ini");
	}

	void RemoveIfExists(const std::filesystem::path& path)
	{
		std::error_code ec;
		std::filesystem::remove(path, ec);
	}
}

TEST_CASE("CConfigManager - persist and read typed values")
{
	const auto path = MakeTempConfigPath("typed_values");
	RemoveIfExists(path);

	{
		Monoworks::CConfigManager cfg(path.string().c_str());
		cfg.RegisterSection("Section 1");
		cfg.RegisterValue("Section 1", "Boolean", "true");
		cfg.RegisterValue("Section 1", "Integer", "1");
		cfg.RegisterValue("Section 1", "Float", "2.5");
		cfg.RegisterValue("Section 1", "String", "Test");
		cfg.Flush();

		CHECK(cfg.Get<bool>("Section 1", "Boolean") == true);
		CHECK(cfg.Get<int>("Section 1", "Integer") == 1);
		CHECK(cfg.Get<float>("Section 1", "Float") == doctest::Approx(2.5f));
		CHECK(cfg.Get<std::string>("Section 1", "String") == "Test");
	}

	RemoveIfExists(path);
}

TEST_CASE("CConfigManager - missing sections and keys return defaults")
{
	const auto path = MakeTempConfigPath("missing_values");
	RemoveIfExists(path);

	{
		Monoworks::CConfigManager cfg(path.string().c_str());
		cfg.RegisterSection("Section 1");
		cfg.Flush();

		CHECK(cfg.Get<std::string>("Section 2", "String") == std::string{});
		CHECK(cfg.Get<int>("Section 2", "Integer") == 0);
		CHECK(cfg.Get<bool>("Section 2", "Boolean") == false);
		CHECK(cfg.Get<float>("Section 2", "Float") == doctest::Approx(-1.0f));
	}

	RemoveIfExists(path);
}

TEST_CASE("CConfigManager - duplicate registrations are ignored")
{
	const auto path = MakeTempConfigPath("duplicates");
	RemoveIfExists(path);

	{
		Monoworks::CConfigManager cfg(path.string().c_str());
		cfg.RegisterSection("Section 1");
		cfg.RegisterValue("Section 1", "Value", "1");
		cfg.RegisterValue("Section 1", "Value", "2");
		cfg.RegisterSection("Section 1");
		cfg.Flush();

		CHECK(cfg.Get<int>("Section 1", "Value") == 1);
	}

	RemoveIfExists(path);
}

TEST_CASE("CConfigManager - flush creates parent directories")
{
	const auto root = std::filesystem::temp_directory_path() /
		std::filesystem::path("monoworks_cfg_nested_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	const auto path = root / "a" / "b" / "settings.ini";

	std::error_code ec;
	std::filesystem::remove_all(root, ec);

	{
		Monoworks::CConfigManager cfg(path.string().c_str());
		cfg.RegisterSection("S");
		cfg.RegisterValue("S", "K", "V");
		cfg.Flush();
	}

	CHECK(std::filesystem::exists(path));

	std::filesystem::remove_all(root, ec);
}


