#pragma once
#include <common/Base.hh>

#include <volk/volk.h>

namespace Monoworks::RHI 
{
	class IGraphicsContext 
	{
		virtual void Init() = 0;
		virtual void Shutdown() = 0;
	};
}
