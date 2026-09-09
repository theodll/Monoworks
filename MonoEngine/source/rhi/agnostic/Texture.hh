#pragma once
#include <common/Base.hh>

#include <rhi/Utils.hh>

namespace Monoworks::RHI
{
	enum ETextureCreationFlagBits
	{
		MW_TEXTURE_CREATION_FLAG_NONE_BIT = 0,
		MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_CREATION_BIT = 0x01,
		MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_VIEW_CREATION_BIT = 0x02,
		MW_TEXTURE_CREATION_FLAG_DISABLE_SAMPLER_CREATION_BIT = 0x04,
		MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_MANAGEMENT_BIT = 0x08,
		MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_VIEW_MANAGEMENT_BIT = 0x10,
		MW_TEXTURE_CREATION_FLAG_DISABLE_SAMPLER_MANAGEMENT_BIT = 0x20,
		MW_TEXTURE_CREATION_FLAG_ENABLE_MEMORY_EXPORTING = 0x40
	};

	using ETextureCreationFlags = flags_t;

	struct STextureCreateInfo 
	{
		SExtent3D Extent;
		ETextureCreationFlags Flags;
		EImageFormat Format;
		EImageUsageFlags Usage;
		EImageLayout ImageLayout;
		EImageAspectFlags AspectMask;
	};

	class ITexture 
	{
	public:
		virtual ~ITexture() = default;

 		NODISCARD virtual MW_NOTHROW u32 GetWidth() const NOEXCEPT = 0;
		NODISCARD virtual MW_NOTHROW u32 GetHeight() const NOEXCEPT = 0;
						  
		NODISCARD virtual MW_NOTHROW u32 ReadPixel( s32 x, s32 y ) NOEXCEPT = 0;

		/**
		 * @brief Transitions the Image layout. Only destination values are required, since current values are stored insize ITexture::Layout and ITexture::PipelineFlags.
		 * frameIndex and threadID are required for semi-explicit commandbuffer management without global state.
		 */
		virtual MW_NOTHROW void TransitionLayout( u32 frameIndex, EImageLayout dstLayout, EPipelineFlags dstPipelineStage, EImageAspectFlags aspectMask, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT = 0;

		EImageLayout Layout = MW_IMAGE_LAYOUT_UNDEFINED;
		EPipelineFlags PipelineFlags = MW_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		
	};

	class ITexture2D : public ITexture
	{
	public: 
		virtual ~ITexture2D() = default;
		NODISCARD static Ref<ITexture2D> Create( const path_t* pPath ) NOEXCEPT;
		NODISCARD static Ref<ITexture2D> Create( s32 width, s32 height ) NOEXCEPT;
		NODISCARD static Ref<ITexture2D> Create( const STextureCreateInfo* pInfo ) NOEXCEPT;
	};
}
