#include <mwpch.hh>
#include "VulkanContext.hh"

#include "VulkanTexture.hh"
#include <rhi/Utils.hh>

namespace Monoworks::RHI
{


	CVulkanTexture2D::CVulkanTexture2D( const path_t path ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		int width{}, height{}, channels{};
		stbi_uc* pixels = stbi_load( path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha );
		MW_ASSERT( pixels, "Failed to load image" );

		m_ImageExtent.Width = width;
		m_ImageExtent.Height = height;

		CreateImage( pixels );
		CreateImageView();
		CreateImageSampler();
	}

	CVulkanTexture2D::CVulkanTexture2D( s32 width, s32 height ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		m_ImageExtent.Width = width;
		m_ImageExtent.Height = height; 

		CreateImageWithoutData();
		CreateImageView();

		auto uploader = CVulkanContext::GetUploader();
		uploader->Begin();
		TransitionImageLayout( uploader->GetCommandBuffer(), &m_Image, MW_FORMAT_B8G8R8A8_SRGB, MW_IMAGE_LAYOUT_UNDEFINED, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		Layout = MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		PipelineFlags = MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		uploader->End();
	}

	CVulkanTexture2D::CVulkanTexture2D( const STextureCreateInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_ImageExtent.Width = pInfo->Extent.Width;
		m_ImageExtent.Height = pInfo->Extent.Height;

		m_GenerateImage = pInfo->GenerateImage;
		m_GenerateImageView = pInfo->GenerateImageView;
		m_GenerateSampler = pInfo->GenerateSampler;

		auto allocator = CVulkanContext::GetAllocator();

		if ( pInfo->GenerateImage ) 
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = ( VkFormat )pInfo->Format;
			imageInfo.extent = { pInfo->Extent.Width, pInfo->Extent.Height, pInfo->Extent.Depth };
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = pInfo->Usage;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.initialLayout = ( VkImageLayout )pInfo->ImageLayout;

			CVulkanContext::GetDevice()->CreateImage( allocator, &m_Image, &imageInfo, &m_ImageAllocation, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

			auto&& uploader = CVulkanContext::GetUploader();
			uploader->Begin();
			TransitionImageLayout( uploader->GetCommandBuffer(), &m_Image, pInfo->Format, MW_IMAGE_LAYOUT_UNDEFINED, MW_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, pInfo->AspectMask );
			Layout = MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			PipelineFlags = MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			uploader->End();

		}

		if ( pInfo->GenerateImageView )
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = ( VkFormat )pInfo->Format;
			viewInfo.subresourceRange.aspectMask = pInfo->AspectMask;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			MW_VK_CHECK( vkCreateImageView( *CVulkanContext::GetDevice()->GetDevice(), &viewInfo, nullptr, &m_ImageView ), "Failed to create Image View" );
		}
		
		if ( pInfo->GenerateSampler )
		{
			CreateImageSampler();
		}
	}

	CVulkanTexture2D::~CVulkanTexture2D() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto allocator = CVulkanContext::GetAllocator();
		auto device = CVulkanContext::GetDevice();
		vmaDestroyImage( *allocator, m_Image, m_ImageAllocation );
		vkDestroyImageView( *device->GetDevice(), m_ImageView, nullptr );
		vkDestroySampler( *device->GetDevice(),  m_Sampler, nullptr);

	}

	NODISCARD u32 CVulkanTexture2D::ReadPixel( s32 x, s32 y ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice();
		auto uploader = CVulkanContext::GetUploader();
		auto allocator = CVulkanContext::GetAllocator();

		u64 size = sizeof( u32 ) * m_ImageExtent.Width * m_ImageExtent.Height;

		if ( m_Fence == VK_NULL_HANDLE || m_StagingBuffer == VK_NULL_HANDLE )
		{
			CreateStagingData();
		}

		uploader->Begin();

		TransitionImageLayout2(
			*uploader->GetCommandBuffer(),
			m_Image,
			( VkImageLayout )Layout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			PipelineFlags,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		
		device->CopyImageToBuffer( uploader->GetCommandBuffer(), &m_Image, &m_StagingBuffer, m_ImageExtent.Width, m_ImageExtent.Height, 1 );
		
		TransitionImageLayout2(
			*uploader->GetCommandBuffer(),
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			( VkImageLayout )Layout,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			PipelineFlags
		);

		uploader->End();

		void* pData;
		vmaMapMemory( *allocator, m_StagingBufferAllocation, &pData );

		u32* pIDs = reinterpret_cast< u32* >( pData );
		u32 id = 0xFFFFFFFF;

		if ( x >= 0 && x < ( s32 )m_ImageExtent.Width && y >= 0 && y < ( s32 )m_ImageExtent.Height )
		{
			id = pIDs[y * m_ImageExtent.Width + x];
		}
		else
		{
			MW_WARN( "Image coordinates out of bounds ({}, {})", ( int )x, ( int )y);
		}

		vmaUnmapMemory( *allocator, m_StagingBufferAllocation );
		return id;
	}

	void CVulkanTexture2D::CreateImage( stbi_uc* pPixelData ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		VkDeviceSize imageSize = m_ImageExtent.Width * m_ImageExtent.Height * 4;

		auto allocator = CVulkanContext::GetAllocator();
		if ( m_StagingBuffer != VK_NULL_HANDLE || m_StagingBufferAllocation != VK_NULL_HANDLE )
		{
			vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );
		}

		CVulkanContext::GetDevice()->CreateBuffer( allocator, &m_StagingBuffer, &m_StagingBufferAllocation, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* pData;
		vmaMapMemory( *allocator, m_StagingBufferAllocation, &pData );
		memcpy( pData, pPixelData, ( size_t )imageSize );
		vmaUnmapMemory( *allocator, m_StagingBufferAllocation );

		stbi_image_free( pPixelData );

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent = { m_ImageExtent.Width, m_ImageExtent.Height, m_ImageExtent.Depth };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;
		Layout = MW_IMAGE_LAYOUT_UNDEFINED;
		PipelineFlags = MW_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		auto uploader = CVulkanContext::GetUploader();
		uploader->Begin();

		CVulkanContext::GetDevice()->CreateImage(allocator, &m_Image, &imageInfo, &m_ImageAllocation, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		TransitionImageLayout( uploader->GetCommandBuffer(), &m_Image, MW_FORMAT_R8G8B8A8_SRGB, MW_IMAGE_LAYOUT_UNDEFINED, MW_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		Layout = MW_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		CVulkanContext::GetDevice()->CopyBufferToImage(uploader->GetCommandBuffer(), &m_StagingBuffer, &m_Image, m_ImageExtent.Width, m_ImageExtent.Height, 1);

		TransitionImageLayout( uploader->GetCommandBuffer(), &m_Image, MW_FORMAT_R8G8B8A8_SRGB, MW_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		Layout = MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		uploader->End();

		vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );
	}

	void CVulkanTexture2D::CreateImageWithoutData() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		imageInfo.extent = { m_ImageExtent.Width, m_ImageExtent.Height, m_ImageExtent.Depth };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		Layout = MW_IMAGE_LAYOUT_UNDEFINED;
		PipelineFlags = MW_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		CVulkanContext::GetDevice()->CreateImage( CVulkanContext::GetAllocator(), &m_Image, &imageInfo, &m_ImageAllocation, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}

	void CVulkanTexture2D::CreateImageView() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice();

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		MW_VK_CHECK( vkCreateImageView( *device->GetDevice(), &viewInfo, nullptr, &m_ImageView), "Failed to create Image View");
	}

	void CVulkanTexture2D::CreateImageSampler() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		auto device = CVulkanContext::GetDevice();

		// Todo [07.03.26, Theo]: Make this flexible and adjustable in settings
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties( *device->GetPhysicalDevice(), &properties );

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		MW_VK_CHECK( vkCreateSampler( *device->GetDevice(), &samplerInfo, nullptr, &m_Sampler ), "Failed to create Texture Sampler" )

	}

	void CVulkanTexture2D::CreateStagingData() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto allocator = CVulkanContext::GetAllocator();
		u64 size = sizeof( u32 ) * m_ImageExtent.Width * m_ImageExtent.Height;

		if ( m_StagingBuffer == VK_NULL_HANDLE || m_StagingBufferSize < size ) {
			if ( m_StagingBuffer != VK_NULL_HANDLE ) {

				vmaDestroyBuffer( *allocator, m_StagingBuffer, m_StagingBufferAllocation );

			}

			CVulkanContext::GetDevice()->CreateBuffer(
				allocator,
				&m_StagingBuffer,
				&m_StagingBufferAllocation,
				size,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			
			);
			m_StagingBufferSize = size;
		}

		if ( m_Fence == VK_NULL_HANDLE )
		{
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			vkCreateFence( *CVulkanContext::GetDevice()->GetDevice(), &fenceInfo, nullptr, &m_Fence );
		}
	}
}
