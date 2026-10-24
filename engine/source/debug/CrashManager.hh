#pragma once
#include <common/Base.hh>
#include <common/Timestep.h>

namespace Monoworks 
{
	constexpr float c_flHeartbeatInterval = 20;

	enum ECrashReporterHeartbeatStatus : byte_t
	{
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_OK = 0x01,
		MW_CRASH_REPORTER_HEARTBEAT_STATUS_READY = 0x02,
	};

	class CCrashManager 
	{
	public:
		static EResult MWAPI_CALL Init() NOEXCEPT;
		static EResult MWAPI_CALL Shutdown() NOEXCEPT;


		static EResult MWAPI_CALL Ready() NOEXCEPT;
		// Not actually sends a heartbeat but counts up to a specified number and when that is 
		// reached it writes a packet into the read part of the pipe
		static EResult MWAPI_CALL Heartbeat( CTimestep ts ) NOEXCEPT;
	private:
		struct Impl;
		static Unique<Impl> m_hImpl;
	};
}
