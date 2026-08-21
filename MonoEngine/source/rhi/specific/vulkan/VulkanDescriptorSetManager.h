#pragma once
#include <common/Base.hh>

namespace Monoworks::RHI 
{
	struct PoolSizes {
		std::vector<std::pair<VkDescriptorType, float>> Sizes = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.0f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.0f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.0f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.0f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.0f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f }
		};
	};

	class CVulkanDescriptorSetManager 
	{
	public:
		void Init();
		void Shutdown();

		void WriteBuffer( VkDescriptorSet hSet, u32 binding, VkBuffer hBuffer, size_t size );
		void WriteImage( VkDescriptorSet hSet, u32 binding, const VkDescriptorImageInfo* pInfo );
		void WriteSampler( VkDescriptorSet hSet, u32 binding, VkSampler hSampler );
		
		VkDescriptorSet Allocate( VkDescriptorSetLayout hSetLayout, u32 maxSetHints MW_NULLABLE = 128 );

		void ResetPools();
		void SetPoolSize( const PoolSizes* pSizes ) { m_PoolSizes = *pSizes; };
	private:
		VkDescriptorPool GrabPool( u32 maxSets );
		VkDescriptorPool CreatePool( u32 maxSets );

		VkDescriptorPool m_CurrentPool = nullptr;

		std::vector<VkDescriptorPool> m_UsedPools;
		std::vector<VkDescriptorPool> m_FreePools;

		PoolSizes m_PoolSizes{};

	};
}
