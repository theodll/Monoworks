#pragma once
#include <common/Base.hh>
#include <rhi/Utils.hh>

#include <boost/unordered_map.hpp>

namespace Monoworks::RHI 
{
	enum EDescriptorType : u8
	{
		MW_DESCRIPTOR_TYPE_SAMPLER,                    // VK_DESCRIPTOR_TYPE_SAMPLER
		MW_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		MW_DESCRIPTOR_TYPE_STORAGE_IMAGE,              // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
		MW_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,       // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
		MW_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,       // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
		MW_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		MW_DESCRIPTOR_TYPE_STORAGE_BUFFER,             // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		MW_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,     // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
		MW_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,     // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
		MW_DESCRIPTOR_TYPE_INPUT_ATTACHMENT            // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
	};

	struct DescriptorBinding 
	{
		std::string Name;
		u32 Binding;
		u32 Count = 1;
		EDescriptorType Type;
		EShaderStage Stage;
	};

	class CVulkanDescriptorLayoutCache
	{
	public:
		void Init();
		void Shutdown();

		VkDescriptorSetLayout CreateLayout( const std::vector<DescriptorBinding>& rBindings );

	private:
		boost::unordered_map<Hash::hash_t, VkDescriptorSetLayout> m_hLayoutCache;

		VkDescriptorSetLayout CreateVulkanLayout( const std::vector<DescriptorBinding>& rBindings );
		Hash::hash_t HashBindings( const std::vector<DescriptorBinding>& rBindings );
	};
}
