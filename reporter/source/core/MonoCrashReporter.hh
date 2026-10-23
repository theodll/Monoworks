#include <Monoworks.hh>
#include <memory>

namespace Monoworks
{
	int MWAPI_CALL MonoCrashReporterMain( int argc, wchar_t** pArgv ) NOEXCEPT;

	class CMonoCrashReporter
	{
	public:
		CMonoCrashReporter() = default;
		virtual ~CMonoCrashReporter() = default;

		void MWAPI_CALL Init();
		void MWAPI_CALL Shutdown();
	private:
		struct Impl;
		std::unique_ptr<Impl> m_pImpl;
	};

}
