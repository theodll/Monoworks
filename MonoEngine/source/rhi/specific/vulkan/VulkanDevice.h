#pragma once
#include <common/Base.hh>

#include <Volk/volk.h>

namespace Monoworks::RHI 
{
	class CVulkanDevice 
	{
		void Init();
		void Shutdown();

		const VkDevice* GetDevice() { return m_Device; };
	private:
		VkDevice m_Device;

	};
}

