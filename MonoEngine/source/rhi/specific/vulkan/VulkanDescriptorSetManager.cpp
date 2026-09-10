#include <mwpch.hh>

#include "VulkanDescriptorSetManager.h"
#include "VulkanUniformBuffer.hh"
#include "VulkanTexture.hh"

#include "VulkanContext.hh"

namespace Monoworks::RHI 
{
	void CVulkanDescriptorSetManager::InitImpl()
	{

	};

	void CVulkanDescriptorSetManager::ShutdownImpl()
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Shutdown CVulkanDescriptorSetManager" );

		auto device = *CVulkanContext::GetDevice()->GetDevice();
		if ( m_CurrentPool )
			vkDestroyDescriptorPool( device, m_CurrentPool, CVulkanContext::GetCallbacks() );
		m_CurrentPool = nullptr;

		for ( auto descriptorPool : m_UsedPools )
			if ( descriptorPool )
				vkDestroyDescriptorPool( device, descriptorPool, CVulkanContext::GetCallbacks() );

		for ( auto descriptorPool : m_FreePools )
			if ( descriptorPool )
				vkDestroyDescriptorPool( device, descriptorPool, CVulkanContext::GetCallbacks() );

		m_FreePools.clear();
		m_UsedPools.clear();
	};

	void CVulkanDescriptorSetManager::WriteBufferImpl( DescriptorHandle pDescriptor, u32 binding, void* pBuffer, size_t size ) 
	{
		MW_PROFILE_FUNC;
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ( VkBuffer )pBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = size;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = ( VkDescriptorSet )pDescriptor;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets( *CVulkanContext::GetDevice()->GetDevice(), 1, &write, 0, nullptr );
	};

	// Only writes the Image!
	void CVulkanDescriptorSetManager::WriteImageImpl( DescriptorHandle pDescriptor, u32 binding, Ref<ITexture2D> hTexture )
	{
		MW_PROFILE_FUNC;

		auto vkTex = hTexture.As<CVulkanTexture2D>();

		VkDescriptorImageInfo info{};
		info.imageView = *vkTex->GetImageView();
		info.imageLayout =  static_cast<VkImageLayout>(vkTex->Layout);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = ( VkDescriptorSet )pDescriptor;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write.pImageInfo = &info;

		vkUpdateDescriptorSets( *CVulkanContext::GetDevice()->GetDevice(), 1, &write, 0, nullptr );
	};

	// Only writes the Sampler! 
	void CVulkanDescriptorSetManager::WriteSamplerImpl( DescriptorHandle pDescriptor, u32 binding, Ref<ITexture2D> hTexture )
	{
		MW_PROFILE_FUNC;

		auto vkTex = hTexture.As<CVulkanTexture2D>();

		VkDescriptorImageInfo info{};
		info.sampler = *vkTex->GetSampler();

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = ( VkDescriptorSet )pDescriptor;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write.pImageInfo = &info;

		vkUpdateDescriptorSets( *CVulkanContext::GetDevice()->GetDevice(), 1, &write, 0, nullptr );
	};

	DescriptorHandle CVulkanDescriptorSetManager::AllocateImpl( DescriptorSignature pSetLayout, u32 maxSetHints )
	{
		MW_PROFILE_FUNC;

		if ( !pSetLayout )
		{
			MW_API_ERROR( "Passed invalid Descriptor Signature." );
			return;
		}


		if ( m_CurrentPool == nullptr ) {
			m_CurrentPool = GrabPool( maxSetHints );
			m_UsedPools.push_back( m_CurrentPool );
		}

		VkDescriptorSet set = VK_NULL_HANDLE;
		VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_CurrentPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = ( VkDescriptorSetLayout* )&pSetLayout;

		VkResult res = vkAllocateDescriptorSets( *CVulkanContext::GetDevice()->GetDevice(), &allocInfo, &set );
		if ( res == VK_SUCCESS ) return set;

		if ( res == VK_ERROR_FRAGMENTED_POOL || res == VK_ERROR_OUT_OF_POOL_MEMORY ) {
			m_CurrentPool = GrabPool( ( ( ( 2u * maxSetHints ) > ( 64u ) ) ? ( 2u * maxSetHints ) : ( 64u ) ) );
			m_UsedPools.push_back( m_CurrentPool );

			allocInfo.descriptorPool = m_CurrentPool;
			if ( vkAllocateDescriptorSets( *CVulkanContext::GetDevice()->GetDevice(), &allocInfo, &set ) != VK_SUCCESS )
				MW_ASSERT( false, "Failed to allocate Descriptor Sets." )
				return set;
		}

		if ( res != VK_SUCCESS )
			MW_ASSERT( false, "Failed to allocate Descripor Sets {}.", res );

		return nullptr;
	};


	void CVulkanDescriptorSetManager::ResetPools()
	{
		MW_PROFILE_FUNC;
		for ( auto p : m_UsedPools )
		{
			vkResetDescriptorPool( *CVulkanContext::GetDevice()->GetDevice(), p, 0 );
			m_FreePools.emplace_back( p );
		};
		m_UsedPools.clear();
		m_CurrentPool = VK_NULL_HANDLE;
	};

	VkDescriptorPool CVulkanDescriptorSetManager::GrabPool( u32 maxSets ) 
	{
		MW_PROFILE_FUNC;
		if ( !m_FreePools.empty() ) {
			VkDescriptorPool pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		return CreatePool( maxSets );
	};

	VkDescriptorPool CVulkanDescriptorSetManager::CreatePool( u32 maxSets ) 
	{
		MW_PROFILE_FUNC;
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve( m_PoolSizes.Sizes.size() );

		for ( auto [type, weight] : m_PoolSizes.Sizes )
		{
			VkDescriptorPoolSize s{};
			s.type = type;
			s.descriptorCount = ( u32 )std::ceil( weight * maxSets );
			poolSizes.push_back( s );
		}

		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.flags = 0;
		info.maxSets = maxSets;
		info.poolSizeCount = ( u32 )poolSizes.size();
		info.pPoolSizes = poolSizes.data();

		VkDescriptorPool pool = VK_NULL_HANDLE;
		if ( vkCreateDescriptorPool( *CVulkanContext::GetDevice()->GetDevice(), &info, nullptr, &pool ) != VK_SUCCESS )
			MW_ASSERT( false, "Failed to create Descriptor Pools" )
		
		return pool;
	};

	void CVulkanDescriptorSetManager::WriteUniformBufferImpl( DescriptorHandle hDescriptor, u32 binding, Ref<IUniformBuffer> hUniformBuffer )
	{
		MW_PROFILE_FUNC;
		auto vkBuf = hUniformBuffer.As<CVulkanUniformBuffer>();

		WriteBufferImpl( hDescriptor, binding, vkBuf->GetVulkanBuffer(), vkBuf->GetSize() );

	}


}
