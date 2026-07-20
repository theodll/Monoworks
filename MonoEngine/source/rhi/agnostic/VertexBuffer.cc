#include <mwpch.hh>
#include "VertexBuffer.hh"

#include <core/Application.hh>
#include <rhi/specific/vulkan/VulkanVertexBuffer.hh>

namespace Monoworks::RHI
{
	void CBufferLayout::CalculateOffsetAndStride()
	{
		u32 offset = 0;
		m_Stride = 0;
		for (auto& element : m_Elements)
		{
			element.Offset = offset;
			offset += element.Size;
			m_Stride += element.Size;
		}
	};


	Ref<IVertexBuffer> IVertexBuffer::Create(void* vertexData, u64 vertexCount, u64 vertexStride, bool autoupload) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		u64 bytes = vertexCount * vertexStride;
		switch (CApplication::GetGraphicsAPI())
		{
			case MW_GAPI_NONE:    return nullptr;
			case MW_GAPI_VULKAN:  return Ref<CVulkanVertexBuffer>::Create(vertexData, bytes, 0, autoupload);
		}
		MW_ASSERT(false, "Unknown Graphics API");
		return nullptr;
	}

}