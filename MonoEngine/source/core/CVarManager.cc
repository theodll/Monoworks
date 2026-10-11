#include "CVarManager.hh"

#include <charconv>
#include <format>
#include <fstream>
#include <map>
#include <string>

namespace Monoworks
{
	namespace
	{
		using Registry = std::map<std::string, SCVar*, std::less<>>;

		Registry& GetRegistry() noexcept
		{
			static Registry* registry = new Registry;
			return *registry;
		}

		float ParseFloat( std::string_view text ) noexcept
		{
			float value = 0.0f;
			const auto [ptr, ec] = std::from_chars( text.data(), text.data() + text.size(), value );
			return ec == std::errc{} ? value : 0.0f;
		}
	}

	SCVar::~SCVar()
	{
		CCvarManager::Unregister( this );
	}

	void CCvarManager::Init() noexcept
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CCvarManager" );
	}

	void CCvarManager::Shutdown() noexcept
	{
		MW_PROFILE_FUNC;
		Serialize( "cvar_config.cfg" );
		MW_INFO( "Shutdown CCvarManager" );
	}

	void CCvarManager::RegisterVariable( SCVar* var ) noexcept
	{
		MW_PROFILE_FUNC;

		if ( !var || var->Name.empty() )
		{
			MW_WARN( "Unable to define cvar: null pointer or empty name" );
			return;
		}

		const auto [it, inserted] = GetRegistry().try_emplace( var->Name, var );
		if ( !inserted )
		{
			MW_WARN( "Unable to define cvar: {}: Already defined", var->Name );
			return;
		}

		var->Value = ParseFloat( var->String );

		MW_INFO( "Register variable {} with string \"{}\" and value \"{}\"", var->Name, var->String, var->Value );
	}

	void CCvarManager::Unregister( SCVar* var ) noexcept
	{
		Registry& registry = GetRegistry();
		const auto it = registry.find( var->Name );

		if ( it != registry.end() && it->second == var )
			registry.erase( it );
	}

	void CCvarManager::Set( std::string_view varName, const std::string& value ) noexcept
	{
		MW_PROFILE_FUNC;

		SCVar* var = Find( varName );
		if ( !var )
		{
			MW_WARN( "Did not find cvar {}", varName );
			return;
		}

		// Note [14.08.26, Theo]: Used by the Quake Engine to broadcast cvar changes to the server (https://github.com/id-Software/Quake/blob/master/WinQuake/cvar.c, line 123)
		MAYBE_UNUSED const bool changed = ( var->String != value );

		var->String = value;
		var->Value = ParseFloat( value );
	}

	void CCvarManager::SetValue( std::string_view varName, float value ) noexcept
	{
		MW_PROFILE_FUNC;
		Set( varName, std::format( "{}", value ) );
	}

	float CCvarManager::GetValue( std::string_view varName ) noexcept
	{
		MW_PROFILE_FUNC;

		const SCVar* var = Find( varName );
		if ( !var )
		{
			MW_WARN( "Did not find cvar {}", varName );
			return 0.0f;
		}
		return var->Value;
	}

	std::string CCvarManager::GetString( std::string_view varName ) noexcept
	{
		MW_PROFILE_FUNC;

		const SCVar* var = Find( varName );
		if ( !var )
		{
			MW_WARN( "Did not find cvar {}", varName );
			return {};
		}
		return var->String;
	}

	void CCvarManager::Serialize( const std::filesystem::path& filePath )
	{
		MW_PROFILE_FUNC;

		std::ofstream file( filePath );
		if ( !file )
		{
			MW_ERROR( "CCvarManager::Serialize: unable to open {}", filePath.string() );
			return;
		}

		for ( const auto& [name, var] : GetRegistry() )
			if ( var->Archive )
				file << name << " \"" << var->String << "\"\n";
	}

	SCVar* CCvarManager::Find( std::string_view varName )
	{
		const Registry& registry = GetRegistry();
		const auto it = registry.find( varName );
		return it != registry.end() ? it->second : nullptr;
	}
}