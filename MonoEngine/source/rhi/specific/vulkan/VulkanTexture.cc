#include <mwpch.hh>
#include "VulkanContext.hh"

#include "VulkanTexture.hh"

namespace Monoworks::RHI 
{
	static void TransitionImageLayout(
		VkCommandBuffer* pCmdBuffer,
		VkImage* pImage,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
	)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = *pImage;
		barrier.subresourceRange.aspectMask = aspectMask;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage{};
		VkPipelineStageFlags destinationStage{};

		if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			MW_ASSERT( false, "Unsupported layout transition" );
		}

		vkCmdPipelineBarrier(
			*pCmdBuffer,
			sourceStage,
			destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

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
		Layout 
		uploader->End();
	}

	CVulkanTexture2D::CVulkanTexture2D( const STextureCreateInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	NODISCARD u32 CVulkanTexture2D::ReadPixel( s32 x, s32 y ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanTexture2D::CreateImage( stbi_uc* pPixelData ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanTexture2D::CreateImageWithoutData() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanTexture2D::CreateImageView() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanTexture2D::CreateImageSampler() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	void CVulkanTexture2D::CreateStagingData() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

}
