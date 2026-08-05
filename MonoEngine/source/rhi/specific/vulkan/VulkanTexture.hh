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

		CVulkanTexture2D( const CVulkanTexture2D& ) = delete;
		CVulkanTexture2D& operator=( const CVulkanTexture2D& ) = delete;

		NODISCARD u32 ReadPixel( s32 x, s32 y ) NOEXCEPT override;

		NODISCARD u32 GetWidth() const NOEXCEPT override { return m_ImageExtent.Width; };
		NODISCARD u32 GetHeight() const NOEXCEPT override { return m_ImageExtent.Height; };

		NODISCARD VkSampler* GetSampler() NOEXCEPT { return &m_Sampler; }
		NODISCARD VkImage* GetImage() NOEXCEPT { return &m_Image; }
		NODISCARD VkImageView* GetImageView() NOEXCEPT { return &m_ImageView; }

		NODISCARD VmaAllocation* GetVmaAllocation() NOEXCEPT { return &m_ImageAllocation; }

		void SetSampler( VkSampler* pSampler ) { MW_WARN( "Manually overwriting the Sampler is dangerous and may lead to undefined behaviour. Only use in certain situations" ); m_Sampler = *pSampler; }
		void SetImage( VkImage* pImage ) { MW_WARN( "Manually overwriting the Image is dangerous and may lead to undefined behaviour. Only use in certain situations" ); m_Image = *pImage; }
		void SetImageView( VkImageView* pImageView ) { MW_WARN( "Manually overwriting the Image View is dangerous and may lead to undefined behaviour. Only use in certain situations" ); m_ImageView = *pImageView; }

	private:
		void CreateImage( stbi_uc* pPixelData ) NOEXCEPT;
		void CreateImageWithoutData() NOEXCEPT;
		void CreateImageView() NOEXCEPT;
		void CreateImageSampler() NOEXCEPT;
		void CreateStagingData() NOEXCEPT;

		bool m_EnableMemoryExporting = false;

		bool m_GenerateImage = true;
		bool m_GenerateImageView = true;
		bool m_GenerateSampler = true;

		// to explicitly manage the memory of the handles (only current use is with the swapchain)
		bool m_ManageImage = true;
		bool m_ManageImageView = true; 
		bool m_ManageSampler = true;

		// TODO: Move to asset manager.
		path_t m_Path;

		SExtent3D m_ImageExtent;

		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VmaAllocation m_ImageAllocation = VK_NULL_HANDLE;

		VkBuffer m_StagingBuffer = VK_NULL_HANDLE;
		VmaAllocation m_StagingBufferAllocation = VK_NULL_HANDLE;
		VkFence m_Fence;

		u64 m_StagingBufferSize;

	};
}
