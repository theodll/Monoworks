#include <core/MonoCrashReporter.hh>
#include <core/Utils.hh>

#include <Version.h>

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

		ApplicationMetaDumpData* pMeta = rp.GetApplicationMetaDumpData();
		rp.WaitForApplicationMeta( pMeta );

		rp.WaitForGraphicsVendor( pMeta );
		rp.WaitForReadyStatus();

		rp.CheckHeartbeat();

		return 0;
	}

	int CMonoCrashReporter::WaitForApplicationMeta( ApplicationMetaDumpData* pMeta ) noexcept
	{
		MW_CRASH_REPORTER_INFO( "Wait for Application Meta." );

		struct Meta 
		{
			ECrashManagerMetaDataFlags nFlags;
			EGraphicsAPI nGraphicsAPI;
			Version EngineVersion;
			Version ClientVersion;
		};
		Meta meta;
		size_t nBytesRead = 0;

		if ( int res = ReadPipe( &meta, sizeof meta, &nBytesRead ); !res )
		{ 
			MW_CRASH_REPORTER_ERROR("Failed to read heartbeat pipe (0x{:x})", res );
			return -1;
		}
		pMeta->ClientVersion = meta.ClientVersion;
		pMeta->EngineVersion = meta.EngineVersion;
		pMeta->GitBranch = MW_GIT_BRANCH;
		pMeta->GitHash = MW_GIT_COMMIT_HASH;
		pMeta->nGraphicsAPI = meta.nGraphicsAPI;

		MW_CRASH_REPORTER_INFO( "Received Application Meta." );

		return 0;
	}

	int CMonoCrashReporter::WaitForGraphicsVendor( ApplicationMetaDumpData* pMeta ) noexcept
	{
		MW_CRASH_REPORTER_INFO("Wait for Graphics Vendor.");

		EGraphicsVendorFlags nFlags = MW_GRAPHICS_VENDOR_UNDEFINED;
		size_t nBytesRead = 0;

		if ( int res = ReadPipe( &nFlags, sizeof nFlags, &nBytesRead ); !res )
		{
			MW_CRASH_REPORTER_ERROR("Failed to read Heartbeat Pipe (0x{:x})", res);
			return -1;
		}

		if ( nFlags == MW_GRAPHICS_VENDOR_UNDEFINED )
		{
			MW_CRASH_REPORTER_ERROR("Graphics Vendor Flag is undefined.");
			return -1;
		}

		MW_CRASH_REPORTER_INFO("Received Graphics Vendor: {}", (int)nFlags);
		pMeta->GraphicsVendor |= nFlags;

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
		return 0;
	}

	int CMonoCrashReporter::Dump() noexcept
	{
		MW_CRASH_REPORTER_INFO("Dumping Engine.");

		PreDumpSteps();

		int nRes = DumpCore();
		return nRes;

	}

	int CMonoCrashReporter::HeartbeatFailed() noexcept
	{
		Dump();
		TerminateEngine();
		return 0;
	}

	}
