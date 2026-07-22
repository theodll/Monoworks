#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/Texture.hh>
#include <rhi/Utils.hh>

#include <volk/volk.h>
#include <vk_mem_alloc.h>
#include <stb/stb_image.h>

namespace Monoworks::RHI 
{
	class CVulkanTexture2D : public ITexture2D 
	{
	public:
		CVulkanTexture2D( const path_t path ) NOEXCEPT;
		CVulkanTexture2D( s32 width, s32 height ) NOEXCEPT;
		CVulkanTexture2D( const STextureCreateInfo* pInfo ) NOEXCEPT;

		~CVulkanTexture2D() NOEXCEPT;

		NODISCARD u32 ReadPixel( s32 x, s32 y ) NOEXCEPT override;

		NODISCARD u32 GetWidth() const NOEXCEPT override { return m_ImageExtent.Width; };
		NODISCARD u32 GetHeight() const NOEXCEPT override { return m_ImageExtent.Height; };
		NODISCARD VkSampler* GetSampler() NOEXCEPT { return &m_Sampler; }
		NODISCARD VkImage* GetImage() NOEXCEPT { return &m_Image; }
		NODISCARD VkImageView* GetImageView() NOEXCEPT { return &m_ImageView; }

	private:
		void CreateImage( stbi_uc* pPixelData ) NOEXCEPT;
		void CreateImageWithoutData() NOEXCEPT;
		void CreateImageView() NOEXCEPT;
		void CreateImageSampler() NOEXCEPT;
		void CreateStagingData() NOEXCEPT;

		// TODO: Move to asset manager.
		path_t m_Path;

		SExtent3D m_ImageExtent;

		VkImage m_Image;
		VkImageView m_ImageView;
		VkSampler m_Sampler;
		VmaAllocation m_ImageAllocation;

		VkBuffer m_StagingBuffer;
		VmaAllocation m_StagingBufferAllocation;
		VkFence m_Fence;

		u64 m_StagingBufferSize;

	};
}
