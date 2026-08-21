#include <mwpch.hh>

#include "VulkanDescriptorLayoutCache.h"


namespace Monoworks::RHI 
{
	constexpr static VkDescriptorType ToVulkanDescriptorType( EDescriptorType type )
	{
		switch ( type )
		{
		case MW_DESCRIPTOR_TYPE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;

		case MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		case MW_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		case MW_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		case MW_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

		case MW_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

		case MW_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		case MW_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

		case MW_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		case MW_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

		case MW_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

		default:
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	constexpr static VkShaderStageFlags ToVulkanShaderStage( EShaderStage stage )
	{
		VkShaderStageFlags flags; 
	}

	void CVulkanDescriptorLayoutCache::Init()
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanDescriptorLayoutCache::Shutdown()
	{
		MW_PROFILE_FUNC;
	}

	VkDescriptorSetLayout CVulkanDescriptorLayoutCache::CreateLayout( const std::vector<DescriptorBinding>& rBindings )
	{
		MW_PROFILE_FUNC;
	}

	VkDescriptorSetLayout CVulkanDescriptorLayoutCache::CreateVulkanLayout( const std::vector<DescriptorBinding>& rBindings )
	{
		MW_PROFILE_FUNC;
	}

	Hash::hash_t CVulkanDescriptorLayoutCache::HashBindings( const std::vector<DescriptorBinding>& rBindings )
	{
		MW_PROFILE_FUNC;
	}

}
