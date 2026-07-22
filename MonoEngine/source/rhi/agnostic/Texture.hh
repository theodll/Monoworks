#pragma once
#include <common/Base.hh>

#include <rhi/Types.hh>

namespace Monoworks::RHI 
{
	struct STextureCreateInfo 
	{
		EImageFormat Format;
		EImageUsageFlags Usage;
		SExtent3D Extent;
		EImageLayout ImageLayout;
		EImageAspectFlags AspectMask;
	};

	class ITexture 
	{
	public:
		virtual ~ITexture() = default;

 		NODISCARD virtual u32 GetWidth() const NOEXCEPT = 0;
		NODISCARD virtual u32 GetHeight() const NOEXCEPT = 0;

		NODISCARD virtual u32 ReadPixel( s32 x, s32 y ) NOEXCEPT = 0;

		EImageLayout Layout;
		
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
