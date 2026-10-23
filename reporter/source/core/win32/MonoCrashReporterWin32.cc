#include <Windows.h>
#include "../MonoCrashReporter.hh"
#include <core/Utils.hh>

#include <string>
#include <print>

#define MAYBE_UNUSED [[maybe_unused]]

int WINAPI wWinMain( MAYBE_UNUSED HINSTANCE hInstance, MAYBE_UNUSED HINSTANCE hPrevInstance, MAYBE_UNUSED PWSTR pCmdLine, MAYBE_UNUSED int nCmdShow )
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW( GetCommandLineW(), &argc);
	auto res = Monoworks::MonoCrashReporterMain( argc, argv );
	LocalFree( argv );
	return res;
};

namespace Monoworks
{

	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept
	{
		CMonoCrashReporter rp;
		if ( rp.Init( argc, pArgv ) < 0 )
			return -1;

		return 0;
	}


	struct CMonoCrashReporter::Impl
	{
		HANDLE	HeartbeatPipe;
		HANDLE	EngineProcess; 
		pid_t	EnginePID;

	};

	CMonoCrashReporter::CMonoCrashReporter() noexcept
		: m_pImpl(::std::make_unique<CMonoCrashReporter::Impl>())
	{}

	int CMonoCrashReporter::Init( int argc, wchar_t** pArgv ) noexcept
	{
		if ( argc > 1 )
		{
			MW_CRASH_REPORTER_ERROR( "Crash Reporter Process: Insufficient count of commandline arguments." );
			return -1;
		}
		m_pImpl->EnginePID = std::stoi( std::wstring(pArgv[1]) );

		m_pImpl->EngineProcess = OpenProcess( PROCESS_VM_READ | PROCESS_VM_OPERATION | SYNCHRONIZE, FALSE, m_pImpl->EnginePID );
		if ( !m_pImpl->EngineProcess )
		{
			MW_CRASH_REPORTER_ERROR( "Crash Reporter Process: Failed to Open Engine Process ({}).", GetLastError() );
			return -1;
		}

		m_pImpl->HeartbeatPipe = GetStdHandle( STD_INPUT_HANDLE );

		return 0;

	}
}
