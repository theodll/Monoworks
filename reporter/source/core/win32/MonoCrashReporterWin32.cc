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
		std::ios::sync_with_stdio( false );
		std::cout.rdbuf()->pubsetbuf( nullptr, 0 ); 
		setvbuf( stdout, nullptr, _IONBF, 0 );

		CMonoCrashReporter rp;
		if ( rp.Init( argc, pArgv ) < 0 )
			return -1;

		rp.CheckHeartbeat();
	

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
		MW_CRASH_REPORTER_INFO("DILDO !!!");
		if ( argc < 2 )
		{
			MW_CRASH_REPORTER_ERROR( "Insufficient count of commandline arguments." );
			return -1;
		}
		m_pImpl->EnginePID = std::stoi( std::wstring(pArgv[1]) );

		m_pImpl->EngineProcess = OpenProcess( PROCESS_VM_READ | PROCESS_VM_OPERATION | SYNCHRONIZE, FALSE, m_pImpl->EnginePID );
		if ( !m_pImpl->EngineProcess )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to Open Engine Process ({}).", GetLastError() );
			return -1;
		}

		m_pImpl->HeartbeatPipe = GetStdHandle( STD_INPUT_HANDLE );

		return 0;

	}

	int CMonoCrashReporter::CheckHeartbeat() noexcept 
	{
		unsigned char heartbeat[64];
		while ( true )
		{
			// more than plenty, the heartbeat is 1 byte large.
			memset( &heartbeat, 0, sizeof heartbeat ); 
			DWORD bytesRead = 0;
			if ( !ReadFile( m_pImpl->HeartbeatPipe, heartbeat, sizeof( unsigned char ), &bytesRead, nullptr ) ) 
			{
				MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe ({})", GetLastError() );
				return -1;
			};
			MW_CRASH_REPORTER_INFO( "Received Heartbeat" );
			MessageBoxA( nullptr, std::format("{}", heartbeat).data(), "received heartbeat", MB_OK | MB_ICONERROR);
		}

		MW_CRASH_REPORTER_INFO( "DILDO !!!" );

		return 0;
	};

}
