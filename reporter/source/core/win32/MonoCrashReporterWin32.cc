#include <Windows.h>
#include <minidumpapiset.h>
#include <DbgHelp.h>
#include <tlhelp32.h>

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

	CMonoCrashReporter::~CMonoCrashReporter() noexcept = default;

	int CMonoCrashReporter::Init( int argc, wchar_t** pArgv ) noexcept
	{
		if ( argc < 2 )
		{
			MW_CRASH_REPORTER_ERROR( "Insufficient count of commandline arguments." );
			return -1;
		}
		m_pImpl->EnginePID = std::stoi( std::wstring(pArgv[1]) );

		m_pImpl->EngineProcess = OpenProcess( PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION | SYNCHRONIZE | PROCESS_TERMINATE, FALSE, m_pImpl->EnginePID );
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

				if ( heartbeat != MW_HEARTBEAT_SIGNAL_OK )
				{
					MW_CRASH_REPORTER_INFO("Signal other than OK received.");
					if ( heartbeat == MW_HEARTBEAT_SIGNAL_CRASH )
					{
						MW_CRASH_REPORTER_INFO( "Crash Signal received" );
						// If this spot gets reached on Windows, something is wrong.
						HeartbeatFailed();
						return 0;
					}
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

	int CMonoCrashReporter::DumpCore() noexcept
	{
		HANDLE hDumpFile = CreateFileA(
			"monoengine.dmp",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr );
	
		if (!MiniDumpWriteDump(
			m_pImpl->EngineProcess,
			m_pImpl->EnginePID,
			hDumpFile, 
			static_cast<MINIDUMP_TYPE>(
				MiniDumpWithDataSegs
				| MiniDumpWithHandleData
				| MiniDumpWithIndirectlyReferencedMemory
				| MiniDumpWithThreadInfo 
				| MiniDumpWithAvxXStateContext),
			nullptr,
			nullptr,
			nullptr
			))
		{
			MW_CRASH_REPORTER_ERROR( "Failed to create minidump. (0x{:x})", GetLastError() );
			CloseHandle( hDumpFile );
			return -1;
		}
		
		CloseHandle( hDumpFile );
		return  0;
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

	int CMonoCrashReporter::PreDumpSteps() noexcept
	{
		// suspend all threads so no new memory is written.
		HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
		if ( hSnapshot == INVALID_HANDLE_VALUE )
			return -1;

		THREADENTRY32 te;
		te.dwSize = sizeof( THREADENTRY32 );

		if ( Thread32First( hSnapshot, &te ) )
		{
			do
			{
				if ( te.th32OwnerProcessID == m_pImpl->EnginePID )
				{
					HANDLE hThread = OpenThread( THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID );
					if ( hThread != NULL )
					{
						SuspendThread( hThread );
						CloseHandle( hThread );
					}
				}
			} while ( Thread32Next( hSnapshot, &te ) );
		}

		CloseHandle( hSnapshot );
		return 0;
	}

	int CMonoCrashReporter::TerminateEngine() noexcept
	{
		MW_CRASH_REPORTER_INFO("Terminating Engine.");

		HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
		if ( hSnapshot == INVALID_HANDLE_VALUE )
			return -1;

		THREADENTRY32 te;
		te.dwSize = sizeof( THREADENTRY32 );

		if ( Thread32First( hSnapshot, &te ) )
		{
			do
			{
				if ( te.th32OwnerProcessID == m_pImpl->EnginePID )
				{
					HANDLE hThread = OpenThread( THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID );
					if ( hThread != NULL )
					{
						ResumeThread( hThread );
						CloseHandle( hThread );
					}
				}
			} while ( Thread32Next( hSnapshot, &te ) );
		}

		CloseHandle( hSnapshot );

		TerminateProcess( m_pImpl->EngineProcess, 1 );
		return 0;
	}

	int CMonoCrashReporter::WalkStack( std::vector<StackFrame>& rStack, size_t nDepth ) noexcept
	{
		DWORD mainThreadId = 0;
		ULONGLONG minCreateTime = MAXULONGLONG;

		HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
		if ( hSnapshot == INVALID_HANDLE_VALUE ) {
			return 0;
		}

		THREADENTRY32 te32;
		te32.dwSize = sizeof( THREADENTRY32 );

		if ( Thread32First( hSnapshot, &te32 ) ) {
			do {
				if ( te32.th32OwnerProcessID == m_pImpl->EnginePID ) {
					HANDLE hThread = OpenThread( THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID );
					if ( hThread ) {
						FILETIME creationTime, exitTime, kernelTime, userTime;
						if ( GetThreadTimes( hThread, &creationTime, &exitTime, &kernelTime, &userTime ) ) {
							ULONGLONG createTimeInt = ( ( ULONGLONG )creationTime.dwHighDateTime << 32 ) + creationTime.dwLowDateTime;

							if ( createTimeInt < minCreateTime ) {
								minCreateTime = createTimeInt;
								mainThreadId = te32.th32ThreadID;
							}
						}
						CloseHandle( hThread );
					}
				}
			} while ( Thread32Next( hSnapshot, &te32 ) );
		}

		HANDLE hMainThread = OpenThread( THREAD_QUERY_INFORMATION, mainThreadId ); 

		if ( !SymInitializeW( m_pImpl->EngineProcess, nullptr, false ) )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to initialize Symbol Handler. (0x{x:})", GetLastError() );
			return -1;
		};

		

	}
}
