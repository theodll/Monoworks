#include <core/MonoCrashReporter.hh>
#include <core/Utils.hh>

#include <string>
#include <print>
#include <thread>
#include <chrono>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept
	{
		CMonoCrashReporter rp;
		if ( rp.Init( argc, pArgv ) < 0 )
			return -1;

		rp.WaitForGraphicsVendor();

		rp.WaitForReadyStatus();

		rp.CheckHeartbeat();

		return 0;
	}


	CMonoCrashReporter::CMonoCrashReporter() noexcept
	{

	}

	int CMonoCrashReporter::WaitForGraphicsVendor() noexcept
	{
		MW_CRASH_REPORTER_INFO("Wait for Graphics Vendor.");

		EGraphicsVendorFlags nFlags = MW_GRAPHICS_VENDOR_UNDEFINED;
		size_t nBytesRead = 0;

		if ( int res = ReadPipe( &nFlags, sizeof nFlags, &nBytesRead ) < 0 )
		{
			MW_CRASH_REPORTER_ERROR("Failed to read Heartbeat Pipe ({})", res);
			return -1;
		}

		if ( nFlags == MW_GRAPHICS_VENDOR_UNDEFINED )
		{
			MW_CRASH_REPORTER_ERROR("Graphics Vendor Flag is undefined.");
			return -1;
		}

		MW_CRASH_REPORTER_INFO("Received Graphics Vendor: {}", (int)nFlags);
		m_nFlags |= nFlags;

		std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

		return 0;
	}

	int CMonoCrashReporter::WaitForReadyStatus() noexcept
	{
		MW_CRASH_REPORTER_INFO( "Wait for ready status." );
		unsigned char readystatus = 0;
		size_t bytesRead = 0;

		while ( true )
		{
			int res = 0;
			if ( ( res = ReadPipe( &readystatus, sizeof( readystatus ), &bytesRead ) ) != 0 )
			{
				MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe (Win32 Error: {})", res );
				return -1;
			}

			if ( bytesRead > 0 )
			{
				if ( readystatus == MW_HEARTBEAT_SIGNAL_READY )
					break;
				else
				{
					MW_CRASH_REPORTER_ERROR( "Received unknown status code: {:#x}", readystatus );
				}
			}
		}

		MW_CRASH_REPORTER_INFO( "Received ready status code." );
		m_bReady = true;
		return 0;
	}

	int CMonoCrashReporter::Shutdown() noexcept
	{

	}

	int CMonoCrashReporter::Dump() noexcept
	{
		MetaDumpData meta{};
		DumpMeta( &meta );

		DumpGPU();

		DumpCore();
	}

	int CMonoCrashReporter::DumpMeta( MetaDumpData* pDumpData ) noexcept
	{

	}

	int CMonoCrashReporter::DumpGPU() noexcept
	{

	}


	int CMonoCrashReporter::HeartbeatFailed() noexcept
	{
		Dump();
		return 0;
	}

}
