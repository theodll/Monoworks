#pragma once
#include <memory>
#include <chrono>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept;

	constexpr auto c_HeartbeatInterval = std::chrono::milliseconds( 2000 );
	constexpr auto c_HeartbeatTimeout = c_HeartbeatInterval * 5;
	constexpr auto c_PollInterval = std::chrono::milliseconds( 10 );

	struct Version
	{
		uint16_t	nPatch;
		uint8_t		nMajor;
		uint8_t		nMinor;
	};

	enum ECrashManagerMetaDataFlagBits
	{
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_NONE_BIT = 0x0,
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_USE_SDL_BIT = 0x1,
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_USE_QT_BIT = 0x2,
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_HEADLESS_BIT = 0x4, // Not implemented
		MW_CRASH_MANAGER_APPLICATION_META_DATA_FLAG_USE_SWAPCHAIN = 0x8,
	};
	using ECrashManagerMetaDataFlags = uint32_t;

	enum EGraphicsAPI
	{
		MW_GAPI_NONE = 0x0,
		MW_GAPI_VULKAN = 0x10000000
	};

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

	enum EMonoCrashReporterFlagBits 
	{

	};
	using EMonoCrashReporterFlags = uint32_t;

	struct ApplicationMetaData
	{
		std::string GitHash;
		std::string GitBranch;
		Version EngineVersion; // Version of the Engine
		Version ClientVersion; // Version of the Client/Runtime
		ECrashManagerMetaDataFlags nFlags;
		EGraphicsAPI nGraphicsAPI;
	};

	// Redundant at the moment, since it right now only supports x86_64
	enum ECPUArch 
	{
		MW_CPU_ARCH_AMD64,
		MW_CPU_ARCH_INTEL,
		MW_CPU_ARCH_ARM64,
		MW_CPU_ARCH_ARM,
		MW_CPU_ARCH_UNKNOWN
	};

	struct SystemMetaData 
	{
		ECPUArch nCPUArch;
		EMonoCrashReporterFlags nGraphicsVendor;
		std::string OperatingSystemVersionSTR;
	};

	enum EHeartbeatSignals 
	{
		MW_HEARTBEAT_SIGNAL_READY = 0xA0,
		MW_HEARTBEAT_SIGNAL_OK = 0x0A,
		MW_HEARTBEAT_SIGNAL_CRASH = 0xFF
	};

	class CMonoCrashReporter
	{
	public:
		CMonoCrashReporter() noexcept;
		~CMonoCrashReporter() noexcept;
		
		// Command line has to be formatted like this:
		// 1. Application name (standard on most systems)
		// 2. Parent Engine's PID
		int Init( int argc, wchar_t** pArgv ) noexcept;

		// Wait for a heartbeat containing the Graphics Vendor.
		int WaitForApplicationMeta( ApplicationMetaData* pMeta ) noexcept;
		int WaitForGraphicsVendor( ApplicationMetaData* pMeta ) noexcept;
		int WaitForReadyStatus() noexcept;
		int CheckHeartbeat() noexcept;
		int Shutdown() noexcept;

		ApplicationMetaData* GetApplicationMetaDumpData() { return &m_ApplicationMeta; };

	private:
		int Dump() noexcept;

		int PreDumpSteps() noexcept;
		// Has to be called last in the dump process
		// because it forcefully exits the engine on Linux and Mac by sending the SIGSEGV signal
		int DumpCore() noexcept; // Platform Specific (MiniDump on Windows, core dump (SIGSEGV) on Linux and Mac)

		int DumpUserMeta() noexcept;

		int WriteReport() noexcept;

		// Does nothing on linux or macos 
		int TerminateEngine() noexcept; 
		int HeartbeatFailed() noexcept;
		int ReadPipe( void* pBuf, size_t nSize, size_t* pBytesRead );


	private:
		struct Impl;
		::std::unique_ptr<Impl> m_pImpl;

		ApplicationMetaData m_ApplicationMeta;

		EMonoCrashReporterFlags m_nFlags;
		bool m_bReady;
	};

}
