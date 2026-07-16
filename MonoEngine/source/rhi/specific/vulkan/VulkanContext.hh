#pragma once
#include <common/Base.hh>
#include <rhi/agnostic/GraphicsContext.hh>

#include <Volk/volk.h>

namespace Monoworks::RHI 
{
	class CVulkanContext : public IGraphicsContext 
	{
	public:
		void Init() override;
		void Shutdown() override;

		const VkInstance* GetInstance() { return &m_Instance; }

	private:

		VkInstance m_Instance;

	};
}
