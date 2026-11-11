#pragma once
#include <filesystem>
#include <memory>
#include <chrono>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept;

	constexpr auto c_HeartbeatInterval = std::chrono::milliseconds( 2000 );
	constexpr auto c_HeartbeatTimeout = c_HeartbeatInterval * 5;
	constexpr auto c_PollInterval = std::chrono::milliseconds( 10 );

	constexpr auto c_ReportPath = "ReportMeta.toml";

	struct alignas(4) Version
	{
		uint8_t		nMajor;
		uint8_t		nMinor;
		uint16_t	nPatch;

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
		std::string CrashTime; // filled in in DumpCore

		std::string GitHash;
		std::string GitBranch;
		Version EngineVersion; // Version of the Engine
		Version ClientVersion; // Version of the Client/Runtime
		ECrashManagerMetaDataFlags nFlags;
		EGraphicsAPI nGraphicsAPI;

		// MS
		uint64_t nEngineUptime; // filled in in DumpCore



	};

	struct SystemMetaData 
	{
		std::string CPUArch;
		// Windows: LCUVer in regedit
		// Linux: utsname::release
		std::string ReleaseVersion;
		// Windows: product name + display version (both in regedit)
		// Linux/MacOS: utsname::nodename
		std::string ProductName; 

		// KB
		uint64_t nTotalSystemMemory;
		uint64_t nUsedSystemMemory;
		uint64_t nEngineMemory;

		// TODO: Add driver version, vram available and architecture
		EMonoCrashReporterFlags nGraphicsVendor;
	};

	enum EHeartbeatSignals
	{
		MW_HEARTBEAT_SIGNAL_READY = 0xA0,
		MW_HEARTBEAT_SIGNAL_OK = 0x0A,
		MW_HEARTBEAT_GPU_CRASH = 0xFE,
		MW_HEARTBEAT_SIGNAL_CRASH = 0xFF
	};

	// move core/monoworks.dmp into the report directory
	int RelocateDump( const std::filesystem::path& dir ) noexcept;

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
		int WaitForApplicationMeta() noexcept;
		int WaitForGraphicsVendor() noexcept;
		int WaitForReadyStatus() noexcept;
		int CheckHeartbeat() noexcept;
		int Shutdown() noexcept;

	private:
		int Dump() noexcept;

		int PreDumpSteps() noexcept;
		// Has to be called last in the dump process
		// because it forcefully exits the engine on Linux and Mac by sending the SIGSEGV signal
		int DumpCore() noexcept; // Platform Specific (MiniDump on Windows, core dump (SIGSEGV) on Linux and Mac)

		int DumpSystemData( SystemMetaData* pMeta ) noexcept;

		int WriteReport() noexcept;
		void ComposeReport() noexcept;

		// Does nothing on linux or macos 
		int TerminateEngine() noexcept; 
		int HeartbeatFailed() noexcept;
		int ReadPipe( void* pBuf, size_t nSize, size_t* pBytesRead );

		int HandleGPUCrash() noexcept;

	private:
		struct Impl;
		::std::unique_ptr<Impl> m_pImpl;

		std::filesystem::path m_ReportDirectory;


		ApplicationMetaData m_ApplicationMeta;
		SystemMetaData m_SystemMeta;

		EMonoCrashReporterFlags m_nFlags;
		bool m_bReady;
	};

}
