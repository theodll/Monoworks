#pragma once
#include <common/Base.hh>

namespace Monoworks::RHI 
{
	class IUniformBuffer
	{
	public:
		virtual ~IUniformBuffer() NOEXCEPT = default;

		virtual void SetData( void* pData, u32 size, u32 offset = 0 ) NOEXCEPT = 0;
		
		NODISCARD virtual u32 GetSize() NOEXCEPT = 0;

		NODISCARD static Ref<IUniformBuffer> Create( u32 size, u32 offset = 0 ) NOEXCEPT;
	};
}
