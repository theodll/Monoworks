#include <Windows.h>
#include "../MonoCrashReporter.hh"
#include <core/Utils.hh>

#include <string>
#include <print>
#include <thread>

#define MAYBE_UNUSED [[maybe_unused]]

/*
int WINAPI wWinMain( MAYBE_UNUSED HINSTANCE hInstance, MAYBE_UNUSED HINSTANCE hPrevInstance, MAYBE_UNUSED PWSTR pCmdLine, MAYBE_UNUSED int nCmdShow )
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW( GetCommandLineW(), &argc);
	auto res = Monoworks::MonoCrashReporterMain( argc, argv );
	LocalFree( argv );
	return res;
};*/

int wmain( int argc, wchar_t** argv )
{
	return Monoworks::MonoCrashReporterMain( argc, argv );
}

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

		rp.WaitForReadyStatus();

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

	int CMonoCrashReporter::WaitForReadyStatus() noexcept
	{
		MW_CRASH_REPORTER_INFO("Wait for ready status")
		unsigned char readystatus = 0;
		DWORD bytesRead = 0;
		if ( !ReadFile( m_pImpl->HeartbeatPipe, &readystatus, sizeof( readystatus ), &bytesRead, nullptr ) )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe ({})", GetLastError() );
			return -1;
		}

		if ( readystatus != 0x02 )
		{
			MW_CRASH_REPORTER_ERROR( "Invalid first status code. This is suboptimal but should idealy not impact execution.");
		}
		MW_CRASH_REPORTER_INFO( "Received ready status code." );
		m_bReady = true; 
		return 0;

	}

	int CMonoCrashReporter::CheckHeartbeat() noexcept
	{
		using clock_t = std::chrono::steady_clock;

		constexpr auto HEARTBEAT_TIMEOUT = std::chrono::milliseconds( 2500 );
		constexpr auto POLL_INTERVAL = std::chrono::milliseconds( 10 );

		auto lastHeartbeat = clock_t::now();

		unsigned char heartbeat = 0;

		if ( !m_bReady )
		{
			MW_CRASH_REPORTER_ERROR("Heartbeat check invoked before ready status code was send.");
		}

		while ( true )
		{
			DWORD bytesAvailable = 0;

			if ( !PeekNamedPipe( m_pImpl->HeartbeatPipe, nullptr, 0, nullptr, &bytesAvailable, nullptr ) )
			{
				MW_CRASH_REPORTER_ERROR( "Failed to peek Heartbeat Pipe ({})", GetLastError() );
				return -1;
			}

			if ( bytesAvailable > 0 )
			{
				DWORD bytesRead = 0;

				if ( !ReadFile( m_pImpl->HeartbeatPipe, &heartbeat, sizeof( heartbeat ), &bytesRead, nullptr ) )
				{
					MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe ({})", GetLastError() );
					return -1;
				}

				if ( bytesRead == sizeof( heartbeat ) )
				{
					lastHeartbeat = clock_t::now();
				}

				continue;
			}

			if ( WaitForSingleObject( m_pImpl->EngineProcess, 0 ) == WAIT_OBJECT_0 )
			{
				MW_CRASH_REPORTER_ERROR( "Engine process has exited." );
				return -1;
			}

			if ( clock_t::now() - lastHeartbeat >= HEARTBEAT_TIMEOUT )
			{
				MW_CRASH_REPORTER_ERROR( "Heartbeat timeout exceeded." );
				HeartbeatFailed();
				return -1;
			}

			std::this_thread::sleep_for( POLL_INTERVAL );
		}
	}

	int CMonoCrashReporter::HeartbeatFailed() noexcept
	{

	}

}
