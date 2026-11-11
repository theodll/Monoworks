#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	using pid_t = u32;

	class CProcess 
	{
	public:
		CProcess( pid_t pid )
		/**
		 * @brief Enumerates all processes on the system.
		 * @return Array of process identifiers
		 */
		static std::vector<pid_t> MWAPI_CALL Enumerate() NOEXCEPT;

	private:
		pid_t m_PID;

	};

}
