#include <core/MonoCrashReporter.hh>
#include <core/Utils.hh>

#include <Version.h>

#include <string>
#include <print>
#include <thread>
#include <chrono>
#include <filesystem>

#include  <toml++/toml.hpp>
#include <lib/zip.h>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept
	{
		CMonoCrashReporter rp;
		if ( rp.Init( argc, pArgv ) < 0 )
			return -1;

		rp.WaitForApplicationMeta();

		rp.WaitForGraphicsVendor();
		rp.WaitForReadyStatus();

		rp.CheckHeartbeat();

		return 0;
	}

	int CMonoCrashReporter::WaitForApplicationMeta() noexcept
	{
		MW_CRASH_REPORTER_INFO( "Wait for Application Meta." );

		struct alignas(16) Meta 
		{
			ECrashManagerMetaDataFlags nFlags;
			EGraphicsAPI nGraphicsAPI;
			Version EngineVersion;
			Version ClientVersion;
		};
		Meta meta;
		size_t nBytesRead = 0;

		int res = ReadPipe( &meta, sizeof meta, &nBytesRead );
		if ( res != 0 )
		{ 
			MW_CRASH_REPORTER_ERROR("Failed to read heartbeat pipe (0x{:x}) !!", res );
		}

		m_ApplicationMeta.ClientVersion = meta.ClientVersion;
		m_ApplicationMeta.EngineVersion = meta.EngineVersion;
		m_ApplicationMeta.GitBranch = MW_GIT_BRANCH;
		m_ApplicationMeta.GitHash = MW_GIT_COMMIT_HASH;
		m_ApplicationMeta.nGraphicsAPI = meta.nGraphicsAPI;

		MW_CRASH_REPORTER_INFO( "Received Application Meta." );

		return 0;
	}

	int CMonoCrashReporter::WaitForGraphicsVendor() noexcept
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
		m_SystemMeta.nGraphicsVendor |= nFlags;

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
		if ( nRes < 0 )
			return nRes;

		nRes = DumpSystemData( &m_SystemMeta );
		if ( nRes < 0 )
			return nRes;

		return nRes;

	}

	int CMonoCrashReporter::WriteReport() noexcept
	{
		const auto now = std::chrono::system_clock::now();
		m_ReportDirectory = std::filesystem::path{ std::format( "monoworks-report_{:%H-%M-%Y-%m-%d}", now ) };
		std::filesystem::create_directory( m_ReportDirectory );

		auto verString = std::format( "{}.{}.{}", m_ApplicationMeta.EngineVersion.nMajor, m_ApplicationMeta.EngineVersion.nMinor, m_ApplicationMeta.EngineVersion.nPatch );
		auto cverString = std::format( "{}.{}.{}", m_ApplicationMeta.ClientVersion.nMajor, m_ApplicationMeta.ClientVersion.nMinor, m_ApplicationMeta.ClientVersion.nPatch );

		constexpr auto vendorStr = []( EMonoCrashReporterFlags nFlags ) constexpr noexcept
			{
				switch ( nFlags )
				{
				case MW_GRAPHICS_VENDOR_AMD: return "AMD";
				case MW_GRAPHICS_VENDOR_APPLE: return "APPLE";
				case MW_GRAPHICS_VENDOR_INTEL: return "INTEL";
				case MW_GRAPHICS_VENDOR_NVIDIA: return "NVIDIA";
				case MW_GRAPHICS_VENDOR_NVIDIA_OLD: return "NVIDIA (OLD)";
				case MW_GRAPHICS_VENDOR_NONE: return "NONE";
				default: return "INVALID";
				}
			};

		constexpr auto gapiStr = []( EGraphicsAPI nGAPI ) constexpr noexcept
			{
				switch ( nGAPI )
				{
				case MW_GAPI_VULKAN: return "VULKAN";
				case MW_GAPI_NONE: return "NONE";
				default: return "INVALID";
				}
			};

		auto report = toml::table
		{
			{ "Crash-Time",						m_ApplicationMeta.CrashTime },
			{ "Application", toml::table{
					{ "Engine-Version", verString },
					{ "Client-Version", cverString },
					{ "Build-Branch",				MW_GIT_BRANCH },
					{ "Build-Commit-Hash",			MW_GIT_COMMIT_HASH },
					{ "Engine-Uptime",				static_cast<int64_t>(m_ApplicationMeta.nEngineUptime / 1000) },
					{ "Graphics-API",				gapiStr(m_ApplicationMeta.nGraphicsAPI) }
				}
			},
			{ "System", toml::table{
					{ "Operating-System-Name",		m_SystemMeta.ProductName },
					{ "Operating-System-Version",	m_SystemMeta.ReleaseVersion },
					{ "CPU-Architecture",			m_SystemMeta.CPUArch },
					{ "Graphics-Vendor",			vendorStr( m_SystemMeta.nGraphicsVendor ) },
					{ "Memory", toml::table{
							{ "Total",				static_cast<int64_t>(m_SystemMeta.nTotalSystemMemory) },
							{ "Used",				static_cast<int64_t>(m_SystemMeta.nUsedSystemMemory) },
							{ "Engine",				static_cast<int64_t>(m_SystemMeta.nEngineMemory) },
						}
					}
				}
			}
		};

		std::ofstream reportstream( m_ReportDirectory / "report.json" );
		reportstream << toml::json_formatter{ report };
		reportstream.close();

		return 0;

	}

	void CMonoCrashReporter::ComposeReport() noexcept
	{
		MW_CRASH_REPORTER_INFO( "Start packaging report" );

		RelocateDump( m_ReportDirectory );
		namespace fs = std::filesystem;

		if ( fs::exists( "gpudata.mwgpudbg" ) )
		{
			fs::rename( "gpudata.mwgpudbg", m_ReportDirectory / "gpudata.mwgpudbg");
		}

		try 
		{
			std::filesystem::copy( "Monoworks.log", m_ReportDirectory / "Monoworks.log" );
		}
		catch ( const std::runtime_error& e )
		{
			MW_CRASH_REPORTER_ERROR("Failed to relocate Log {}", e.what() );
		}

		int error = 0;

		std::string zipd = std::format( "{}.mwrpt", m_ReportDirectory.string() );

		zip* archive = zip_open(
			zipd.c_str(),
			ZIP_CREATE | ZIP_TRUNCATE,
			&error
		);

		if ( !archive )
			return;

		for ( const auto& entry : fs::recursive_directory_iterator( m_ReportDirectory ) )
		{
			if ( !entry.is_regular_file() )
				continue;

			fs::path relativePath =
				fs::relative( entry.path(), m_ReportDirectory );

			std::string archivePath = relativePath.generic_string();

			zip_source* source = zip_source_file(
				archive,
				entry.path().string().c_str(),
				0,
				-1
			);

			if ( !source )
			{
				zip_discard( archive );
				return;
			}

			int64_t indexs;
			indexs = zip_file_add(
				archive,
				archivePath.c_str(),
				source,
				ZIP_FL_ENC_UTF_8
			);

			if ( indexs < 0 )
			{
				zip_source_free( source );
				zip_discard( archive );
				return;
			}

			if ( zip_set_file_compression(
				archive,
				static_cast< uint64_t >( indexs ),
				ZIP_CM_DEFLATE,
				1
			) < 0 )
			{
				zip_discard( archive );
				return;
			}
		}

		if ( zip_close( archive ) < 0 )
		{
			zip_discard( archive );
			return;
		}

		try
		{
			fs::remove_all( m_ReportDirectory );
		}
		catch ( const std::runtime_error& e )
		{
			MW_CRASH_REPORTER_ERROR("Failed to remove m_ReportDir {}", e.what());
		}


		MW_CRASH_REPORTER_INFO( "Finish packaging report" );

	}

	int CMonoCrashReporter::HeartbeatFailed() noexcept
	{
		Dump();
		TerminateEngine();
		WriteReport();
		ComposeReport();
		return 0;
	}

	}
