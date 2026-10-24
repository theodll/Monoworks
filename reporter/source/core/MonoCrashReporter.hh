#pragma once
#include <memory>

namespace Monoworks
{
	int MonoCrashReporterMain( int argc, wchar_t** pArgv ) noexcept;

	class CMonoCrashReporter
	{
	public:
		CMonoCrashReporter() noexcept;
		virtual ~CMonoCrashReporter() noexcept = default;

		int Init( int argc, wchar_t** pArgv ) noexcept;
		int CheckHeartbeat() noexcept;
		int Shutdown() noexcept;
	private:
		struct Impl;
		::std::unique_ptr<Impl> m_pImpl;
	};

}
