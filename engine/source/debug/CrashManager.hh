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
		// This signal should ideally never be issued on Windows. It is only issued for UNIX Systems 
		// because we can't easily register external signal handlers for processes. On windows any signal should
		// ideally be handled by the crash reporter process.
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_CRASH = 0xFF
	};

	enum EGraphicsVendorFlagBits : unsigned char
	{
		MW_GRAPHICS_VENDOR_UNDEFINED = 0x0,
		MW_GRAPHICS_VENDOR_NVIDIA = 0x01,
		// NVIDIA Cards with architecture released before NVIDIA Pascal, like Maxwell (GTX 970, 980, 750 Ti) or Kepler .
		MW_GRAPHICS_VENDOR_NVIDIA_OLD = 0x02,
		MW_GRAPHICS_VENDOR_AMD = 0x04,
		MW_GRAPHICS_VENDOR_INTEL = 0x08,
		MW_GRAPHICS_VENDOR_APPLE = 0x10, // currently unsupported
		MW_GRAPHICS_VENDOR_NONE = 0x20,
		MW_GRAPHICS_VENDOR_UNKNOWN = 0x40
	};
	using EGraphicsVendorFlags = flags_t;


	class CCrashManager 
	{
	public:
		static EResult MWAPI_CALL Init() NOEXCEPT;
		static EResult MWAPI_CALL Shutdown() NOEXCEPT;

		static EResult MWAPI_CALL SendGraphicsVendor( EGraphicsVendorFlags nVendor ) NOEXCEPT;

		static EResult MWAPI_CALL Ready() NOEXCEPT;
		// Not actually sends a heartbeat but counts up to a specified number and when that is 
		// reached it writes a packet into the read part of the pipe
		static EResult MWAPI_CALL Heartbeat( CTimestep ts ) NOEXCEPT;
	private:
		struct Impl;
		static Unique<Impl> m_hImpl;
	};
}
