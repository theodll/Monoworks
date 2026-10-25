#pragma once
#include <memory>
#include <chrono>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept;

	constexpr auto c_HeartbeatInterval = std::chrono::milliseconds( 2000 );
	constexpr auto c_HeartbeatTimeout = c_HeartbeatInterval * 5;
	constexpr auto c_PollInterval = std::chrono::milliseconds( 10 );

	enum EGraphicsVendorFlags : unsigned char
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
	using EMonoCrashReporterFlags = unsigned char;
	class CMonoCrashReporter
	{
	public:
		CMonoCrashReporter() noexcept;
		virtual ~CMonoCrashReporter() noexcept;
		
		// Commandline has to be formatted like this:
		// 1. Application name (standard on most systems)
		// 2. Parent Engine's PID
		int Init( int argc, wchar_t** pArgv ) noexcept;

		// Wait for a heartbeat containing the Graphics Vendor.
		int WaitForGraphicsVendor() noexcept;
		int WaitForReadyStatus() noexcept;
		int CheckHeartbeat() noexcept;
		int Shutdown() noexcept;

	private:
		int HeartbeatFailed() noexcept;

		int ReadPipe( void* pBuf, size_t nSize, size_t* pBytesRead );

	private:
		struct Impl;
		::std::unique_ptr<Impl> m_pImpl;

		EMonoCrashReporterFlags m_nFlags;
		bool m_bReady;
	};

}
