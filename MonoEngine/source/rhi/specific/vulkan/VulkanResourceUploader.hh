#pragma once
#include <common/Base.hh>

#include <volk/volk.h>

namespace Monoworks::RHI 
{
	class CVulkanResourceUploader 
	{
	public:
		void Init() noexcept;
		void Shutdown() noexcept;

		void Begin() noexcept;
		void End() noexcept;

		[[nodiscard]] VkCommandBuffer* GetCommandBuffer() noexcept { return &m_Commandbuffer; };

	private:
		VkCommandBuffer m_Commandbuffer;
		VkFence m_Fence;
	};
}
