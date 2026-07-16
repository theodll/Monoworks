#pragma once
#include <common/Base.hh>

#include <Volk/volk.h>

namespace Monoworks::RHI 
{
	class IGraphicsContext 
	{
		virtual void Init() = 0;
		virtual void Shutdown() = 0;
	};
}
