#pragma once
#include <common/Base.hh>

namespace Monoworks 
{
	class CTimestep 
	{
	public:
		CTimestep( float flTimestep ) : m_flTimestep( flTimestep ) {};

		CTimestep() = default;

		float GetMilliseconds() NOEXCEPT { return m_flTimestep * 1000; }
		float GetSeconds() NOEXCEPT { return m_flTimestep; }

		operator float() NOEXCEPT { return GetSeconds(); };
	private:
		float m_flTimestep;
	};
}
