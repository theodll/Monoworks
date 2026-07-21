#include <common/Base.hh>

#include <vk_mem_alloc.h>
#include <volk/volk.h>

#include "IndexBuffer.hh"

namespace Monoworks::RHI
{
	NODISCARD Ref<IIndexBuffer> IIndexBuffer::Create( u64 size ) NOEXCEPT
	{

	};

	NODISCARD Ref<IIndexBuffer> IIndexBuffer::Create( void* data, u64 size, u64 offset = 0, bool autoupload = false ) NOEXCEPT 
	{
	};
}