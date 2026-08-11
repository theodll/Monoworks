#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#endif
#include <doctest/doctest.h>

#include <core/ConfigManager.hh>

TEST_CASE("CConfigManager - Access config values")
{
	Monoworks::CConfigManager cfg("TestConfig.cfg");
	cfg.RegisterSection( "Section 1" );
	cfg.RegisterValue( "Section 1", "Boolean", "true");
	cfg.RegisterValue( "Section 1", "Integer", "1" );
	cfg.RegisterValue( "Section 1", "String", "Test" );
	cfg.Flush();

	SUBCASE( "Access boolean value" )
	{
		CHECK( cfg.Get<bool>( "Section 1", "Boolean" ) == true );
	}

	SUBCASE( "Access integer value" )
	{
		CHECK( cfg.Get<int>( "Section 1", "Integer" ) == 1 );
	};

	SUBCASE( "Access string value" )
	{
		CHECK( cfg.Get<std::string>( "Section 1", "String " ) == "Test" );
	}
	
}

