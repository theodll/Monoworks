#include <mwpch.hh>

#include <Windows.h>
#include <debug/CrashManager.hh>


namespace Monoworks 
{
	Unique<CCrashManager::Impl> CCrashManager::m_hImpl;

	struct CCrashManager::Impl 
	{
		HANDLE hWriteHeartbeatPipe;
	};

	EResult MWAPI_CALL CCrashManager::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_hImpl = CUnique<CCrashManager::Impl>::Create();

		HANDLE hReadPipe = nullptr;
		HANDLE hWritePipe = nullptr;

		SECURITY_ATTRIBUTES pipesi{};
		ZeroMemory( &pipesi, sizeof SECURITY_ATTRIBUTES );
		pipesi.bInheritHandle = true;
		pipesi.nLength = sizeof SECURITY_ATTRIBUTES;

		if ( !CreatePipe( &hReadPipe, &hWritePipe, &pipesi, 32 ) )
		{
			MW_ERROR( "Failed to create heartbeat pipe ({}).", GetLastError());
			return MW_ERROR_INITIALIZATION_FAILED;
		}

		
		SetHandleInformation( hWritePipe, HANDLE_FLAG_INHERIT, 0 );

		DWORD enginePid = GetCurrentProcessId();

		std::wstring crashReporterCmd = std::format( L"MonoCrashReporter.exe {}", enginePid );

		STARTUPINFOW startinfo{};
		ZeroMemory( &startinfo, sizeof STARTUPINFOW );
		startinfo.cb = sizeof startinfo;
		startinfo.dwFlags |= STARTF_USESTDHANDLES;
		startinfo.hStdInput = hReadPipe;
		startinfo.hStdError = GetStdHandle( STD_ERROR_HANDLE );
		startinfo.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );

		PROCESS_INFORMATION pi{};
		ZeroMemory( &pi, sizeof pi );

		BOOL res = CreateProcessW(
			nullptr,
			crashReporterCmd.data(),
			nullptr,
			nullptr,
			TRUE,
			0,
			nullptr,
			nullptr,
			&startinfo,
			&pi
		);

		if ( !res )
		{
			MW_ERROR( "Failed to create crash reporter ({}).", GetLastError() );
			CloseHandle( hWritePipe );
			return MW_ERROR_INITIALIZATION_FAILED;
		}

		m_hImpl->hWriteHeartbeatPipe = hWritePipe;

		MW_INFO( "Successfully instantiated MonoCrashReporter" );
		return MW_SUCCESS;

	}

	EResult MWAPI_CALL CCrashManager::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if (!CloseHandle( m_hImpl->hWriteHeartbeatPipe ))
		{
			return MW_ERROR_UNKNOWN;
		}
		return MW_SUCCESS;
	}

	EResult MWAPI_CALL CCrashManager::Heartbeat( CTimestep ts ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		static float counter = 0;
		counter += 10 * ts;

		if ( counter >= c_flHeartbeatInterval )
		{
			counter = 0;
			byte_t status = MW_CRASH_REPORTER_HEARTBEAT_STATUS_OK;
			DWORD bytesWritten = 0;

			if (!WriteFile( m_hImpl->hWriteHeartbeatPipe, &status, sizeof status, &bytesWritten, nullptr ) )
			{
				MW_ERROR( "Failed to write to heartbeat-pipe. This is a sign that MonoCrashReporter has exited early. No further heartbeats will be send." );
				return MW_ERROR_EARLY_EXIT;
			}
			MW_INFO( "Heartbeat send" );

		}

		return MW_SUCCESS;

	}
}
