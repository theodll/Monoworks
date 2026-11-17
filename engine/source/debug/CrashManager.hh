#pragma once
#include <common/Base.hh>
#include <common/Timestep.h>

namespace Monoworks 
{
	constexpr float c_flHeartbeatInterval = 20;

	enum ECrashReporterHeartbeatStatus : byte_t
	{
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_READY = 0xA0,
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_OK = 0x0A,
		// Called when a GPU Crash occured. Signal for the Crash reporter to listen for the next 4 
		// bytes to get the size and then the size amout of bytes to get the crash dump data
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_GPU_CRASH = 0xFE, 
		// This signal should ideally never be issued on Windows. It is only issued for UNIX Systems 
		// because we can't easily register external signal handlers for processes. On windows any signal should
		// ideally be handled by the crash reporter process.
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_CRASH = 0xFF
	};

	enum ECrashManagerMetaDataFlagBits
	{
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_NONE_BIT = 0x0,
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_USE_SDL_BIT = 0x1,
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_USE_QT_BIT = 0x2,
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_HEADLESS_BIT = 0x4, // Not implemented
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_USE_SWAPCHAIN = 0x8,
	};
	using ECrashManagerMetaDataFlags = flags_t; 

	struct alignas(16) CrashManagerApplicationMetadata
	{
		ECrashManagerMetaDataFlags nFlags;
		EGraphicsAPI nGraphicsAPI;
		Version EngineVersion;
		Version ClientVersion;
	};

	class CCrashManager 
	{
	public:
		static EResult MWAPI_CALL Init() NOEXCEPT;
		static EResult MWAPI_CALL Shutdown() NOEXCEPT;

		static EResult MWAPI_CALL SendApplicationMeta( const CrashManagerApplicationMetadata* pMeta ) NOEXCEPT;

		static EResult MWAPI_CALL SendGraphicsVendor( EGraphicsVendorFlags nVendor ) NOEXCEPT;

		static EResult MWAPI_CALL DumpEngineWithGPUCrashData( size_t nCrashDataSize, const void* pCrashData ) NOEXCEPT;

		static EResult MWAPI_CALL Ready() NOEXCEPT;
		// Not actually sends a heartbeat but counts up to a specified number and when that is 
		// reached it writes a packet into the read part of the pipe
		static EResult MWAPI_CALL Heartbeat( CTimestep ts ) NOEXCEPT;
	private:
		bool m_bAlive;

		struct Impl;
		static Unique<Impl> m_hImpl;
	};
}
