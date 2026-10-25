#include <Windows.h>
#include "../MonoCrashReporter.hh"
#include <core/Utils.hh>

#include <string>
#include <print>
#include <thread>


namespace Monoworks
{

	struct CMonoCrashReporter::Impl
	{
		HANDLE	HeartbeatPipe;
		HANDLE	EngineProcess; 
		pid_t	EnginePID;

	};

	CMonoCrashReporter::CMonoCrashReporter() noexcept
		: m_pImpl(::std::make_unique<CMonoCrashReporter::Impl>())
	{}

	CMonoCrashReporter::~CMonoCrashReporter() noexcept
	{

	}

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

	int CMonoCrashReporter::CheckHeartbeat() noexcept
	{
		using clock = std::chrono::steady_clock;

		auto lastHeartbeat = clock::now();

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
				size_t bytesRead = 0;

				if ( !ReadPipe( &heartbeat, sizeof( heartbeat ), &bytesRead ) )
				{
					MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe ({})", GetLastError() );
					return -1;
				}

				if ( bytesRead == sizeof( heartbeat ) )
				{
					lastHeartbeat = clock::now();
				}

				continue;
			}

			if ( WaitForSingleObject( m_pImpl->EngineProcess, 0 ) == WAIT_OBJECT_0 )
			{
				MW_CRASH_REPORTER_ERROR( "Engine process has exited." );
				return -1;
			}

			if ( clock::now() - lastHeartbeat >= c_HeartbeatInterval )
			{
				MW_CRASH_REPORTER_ERROR( "Heartbeat timeout exceeded." );
				HeartbeatFailed();
				return -1;
			}

			std::this_thread::sleep_for( c_PollInterval );
		}
	}

	int CMonoCrashReporter::ReadPipe( void* pBuf, size_t nSize, size_t* pBytesRead )
	{
		DWORD dSize = ( DWORD )nSize;
		DWORD dBytesRead = 0;
		if ( !ReadFile( m_pImpl->HeartbeatPipe, pBuf, dSize, &dBytesRead, nullptr ) ) 
		{
			return GetLastError();
		}

		*pBytesRead = dBytesRead;

		return 0;
	}

}
