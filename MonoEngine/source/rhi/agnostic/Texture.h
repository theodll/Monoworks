#pragma once
#include <common/Base.hh>

namespace Monoworks::RHI 
{
	struct STextureCreateInfo 
	{

	};

	class ITexture 
	{
	public:
		virtual ~ITexture();

		NODISCARD virtual u32 GetWidth() const NOEXCEPT = 0;
		NODISCARD virtual u32 GetHeight() const NOEXCEPT = 0;

		NODISCARD virtual u32 ReadPixel( s32 x, s32 y ) NOEXCEPT = 0;
	};

	class ITexture2D : public ITexture
	{
	public: 
		virtual ~ITexture2D() = default;
		NODISCARD static Ref<ITexture2D> Create( const path_t path ) NOEXCEPT;
		NODISCARD static Ref<ITexture2D> Create( s32 width, s32 height ) NOEXCEPT;
		NODISCARD static Ref<ITexture2D> Create(const STextureCreateInfo* pInfo);
	};
}
