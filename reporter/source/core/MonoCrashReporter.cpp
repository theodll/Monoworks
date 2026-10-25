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

		MW_CRASH_REPORTER_INFO(std::format("Received Graphics Vendor: {}", (int)nFlags));
		m_nFlags |= nFlags;

		std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

		return 0;
	}

	int CMonoCrashReporter::WaitForReadyStatus() noexcept
	{
		MW_CRASH_REPORTER_INFO( "Wait for ready status." );
		unsigned char readystatus = 0;
		size_t bytesRead = 0;
		if ( int res = ReadPipe( &readystatus, sizeof( readystatus ), &bytesRead ) < 0 )
		{
			MW_CRASH_REPORTER_ERROR( "Failed to read Heartbeat Pipe ({})", res );
			return -1;
		}

		if ( readystatus != 0x02 )
		{
			MW_CRASH_REPORTER_ERROR( "Invalid first status code. This is suboptimal but should idealy not impact execution." );
		}
		MW_CRASH_REPORTER_INFO( "Received ready status code." );
		m_bReady = true;
		return 0;

	}

	int CMonoCrashReporter::HeartbeatFailed() noexcept
	{
		return -1;
	}

}
