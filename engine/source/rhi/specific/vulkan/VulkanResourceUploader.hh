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

		NODISCARD VkCommandBuffer* GetCommandBuffer() NOEXCEPT { return &m_Commandbuffer; };

	private:
		std::mutex m_Mutex;

		VkCommandBuffer m_Commandbuffer = nullptr;
		VkFence m_Fence = nullptr;
	};
}
