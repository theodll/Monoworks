#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	class CTimestep 
	{
	public:
		CTimestep( double flTimestep ) : m_flTimestep( flTimestep ) {};

		CTimestep() = default;

		double GetMilliseconds() NOEXCEPT { return m_flTimestep * 1000; }
		double GetSeconds() NOEXCEPT { return m_flTimestep; }

		operator double() NOEXCEPT { return GetSeconds(); };

	private:
		double m_flTimestep;
	};
}
