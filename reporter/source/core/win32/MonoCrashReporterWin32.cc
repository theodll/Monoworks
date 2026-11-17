#include <Windows.h>
#include <Psapi.h>
#include <minidumpapiset.h>
#include <DbgHelp.h>
#include <tlhelp32.h>

#include "../MonoCrashReporter.hh"

#include <core/Utils.hh>

#include <array>
#include <string>
#include <print>
#include <thread>
#include <filesystem>


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

	int CMonoCrashReporter::HandleGPUCrash() noexcept
	{
		size_t nGPUDataSize;
		std::vector<byte_t> buf;

		size_t bytesRead = 0;

		int res;
		if ( ( res = ReadPipe( &nGPUDataSize, sizeof nGPUDataSize, &bytesRead ) ) != 0 )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe (Win32 Error: {})", res );
			return -1;
		}

		buf.resize( nGPUDataSize );
		
		if ( ( res = ReadPipe( buf.data(), nGPUDataSize, &bytesRead) ) != 0 )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe (Win32 Error: {})", res );
			return -1;
		}

		HANDLE hFile = CreateFileA(
			"gpudata.mwgpudbg",
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);

		DWORD bytesWritten = 0;

		if ( !WriteFile( hFile, buf.data(), static_cast<DWORD>(buf.size()), &bytesWritten, nullptr ) )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to write GPU Crash file (Win32 Error: {})", GetLastError() );
			return -1;
		}

		CloseHandle( hFile );

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

				switch ( heartbeat )
				{
				case MW_HEARTBEAT_SIGNAL_OK:
					break;
				case MW_HEARTBEAT_SIGNAL_CRASH:
					MW_CRASH_REPORTER_INFO( "Crash Signal received." );
					// If this spot gets reached on Windows, something is wrong.
					HeartbeatFailed();
					return 0;
				case MW_HEARTBEAT_GPU_CRASH:
					MW_CRASH_REPORTER_INFO( "GPU Crash Signal received." );

					HandleGPUCrash();
					HeartbeatFailed();
					return 0;
				default:
					MW_CRASH_REPORTER_ERROR("Invalid heartbeat signal.");
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
		FILETIME creationTime, exitTime, kernelTime, userTime, currentTime;

		if ( !GetProcessTimes( m_pImpl->EngineProcess, &creationTime, &exitTime, &kernelTime, &userTime ) )
		{
			MW_CRASH_REPORTER_ERROR("Failed to get process times for the engine.");
			return -1;
		}

		ULARGE_INTEGER creation, current;
		GetSystemTimeAsFileTime( &currentTime );
		creation.LowPart = creationTime.dwLowDateTime;
		creation.HighPart = creationTime.dwHighDateTime;
		current.LowPart = currentTime.dwLowDateTime;
		current.HighPart = currentTime.dwHighDateTime;
		
		m_ApplicationMeta.nEngineUptime = ( current.QuadPart - creation.QuadPart ) / 10000;

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

	int CMonoCrashReporter::DumpSystemData( SystemMetaData* pMeta ) noexcept
	{
		auto findStr = []( const char* subkey, const char* valueName ) -> std::string
			{
				DWORD bufferSize = 0;

				LSTATUS status = RegGetValueA(
					HKEY_LOCAL_MACHINE,
					subkey,
					valueName,
					RRF_RT_REG_SZ,
					nullptr,
					nullptr,
					&bufferSize
				);

				if ( status != ERROR_SUCCESS )
				{
					MW_CRASH_REPORTER_ERROR(
						"Failed to find Registry Value Position: {}/{} (Error: {})",
						subkey, valueName, status
					);

					return {};
				}

				std::string value( bufferSize, '\0' );

				status = RegGetValueA(
					HKEY_LOCAL_MACHINE,
					subkey,
					valueName,
					RRF_RT_REG_SZ,
					nullptr,
					value.data(),
					&bufferSize
				);

				if ( status != ERROR_SUCCESS )
				{
					MW_CRASH_REPORTER_ERROR(
						"Failed to read Registry Value: {}/{} (Error: {})",
						subkey, valueName, status
					);
					return {};
				}

				while ( !value.empty() && value.back() == '\0' )
					value.pop_back();

				return value;
			};

		SYSTEM_INFO si{};
		GetSystemInfo( &si );

		switch ( si.wProcessorArchitecture )
		{
		case PROCESSOR_ARCHITECTURE_AMD64:
			pMeta->CPUArch = "x86_64-amd64";
			break;
		case PROCESSOR_ARCHITECTURE_INTEL:
			pMeta->CPUArch = "x86";
			break;
		case PROCESSOR_ARCHITECTURE_ARM:
			pMeta->CPUArch = "arm";
			break;
		case PROCESSOR_ARCHITECTURE_ARM64:
			pMeta->CPUArch = "arm64";
			break;
		default:
			pMeta->CPUArch = "Unknown";
			break;
		};

		constexpr auto subkey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\";
		pMeta->ReleaseVersion = findStr( subkey, "LCUVer" );

		auto productname = findStr( subkey, "ProductName" );
		auto displayver = findStr( subkey, "DisplayVersion" );

		pMeta->ProductName = productname + " " + displayver;

		if ( !GetPhysicallyInstalledSystemMemory( &pMeta->nTotalSystemMemory ) ) 
		{
			return -1;
		};
		
		PROCESS_MEMORY_COUNTERS pm{};
		pm.cb = sizeof pm;

		if ( !K32GetProcessMemoryInfo( m_pImpl->EngineProcess, &pm, sizeof pm ) ) 
		{
			return -1;
		};

		pMeta->nEngineMemory = pm.PagefileUsage / 1024;


		MEMORYSTATUSEX ms{};
		ms.dwLength = sizeof ms;

		if ( !GlobalMemoryStatusEx( &ms ) )
		{
			return -1;
		}
			 
		pMeta->nUsedSystemMemory = (ms.ullTotalPhys - ms.ullAvailPhys) / 1024;

		return 0;

	}


	int RelocateDump( const std::filesystem::path& dir ) noexcept
	{
		std::filesystem::path source = "monoengine.dmp";

		std::filesystem::path dst = dir / source;

		try 
		{
			std::filesystem::rename( source, dst );
		}
		catch ( const std::runtime_error& e ) 
		{
			MW_CRASH_REPORTER_ERROR("Failed to relocate dump {}", e.what() );
		}

		return 0;

	}



}
