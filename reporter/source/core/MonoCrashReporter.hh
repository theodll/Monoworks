#pragma once
#include <memory>
#include <chrono>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept;

	constexpr auto c_HeartbeatInterval = std::chrono::milliseconds( 2000 );
	constexpr auto c_HeartbeatTimeout = c_HeartbeatInterval * 5;

	class CMonoCrashReporter
	{
	public:
		CMonoCrashReporter() noexcept;
		virtual ~CMonoCrashReporter() noexcept = default;

		int Init( int argc, wchar_t** pArgv ) noexcept;
		int WaitForReadyStatus() noexcept;
		int CheckHeartbeat() noexcept;
		int Shutdown() noexcept;

	private:
		int HeartbeatFailed() noexcept;

	private:
		struct Impl;
		::std::unique_ptr<Impl> m_pImpl;

		bool m_bReady;
	};

}
