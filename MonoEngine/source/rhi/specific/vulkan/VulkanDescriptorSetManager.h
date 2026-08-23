#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/DescriptorManager.hh>

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

	class CVulkanDescriptorSetManager : public CDescriptorManager
	{
	public:
		void InitImpl() NOEXCEPT override;
		void ShutdownImpl() NOEXCEPT override;


		void WriteBufferImpl(	DescriptorHandle pDescriptor, u32 binding, void* pBuffer, size_t size ) override;

		// Only writes the Image!
		void WriteImageImpl(	DescriptorHandle pDescriptor, u32 binding, Ref<ITexture2D> hTexture ) override;

		// Only writes the Sampler! 
		void WriteSamplerImpl(	DescriptorHandle pDescriptor, u32 binding, Ref<ITexture2D> hTexture ) override;

		DescriptorHandle AllocateImpl( DescriptorSignature pSetLayout, u32 maxSetHints MW_NULLABLE = 128 ) override;


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
