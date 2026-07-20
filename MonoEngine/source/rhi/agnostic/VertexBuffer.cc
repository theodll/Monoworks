#include <mwpch.hh>
#include <common/Memory.hh>

#include "VertexBuffer.hh"

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


	Ref<IVertexBuffer> IVertexBuffer::Create(void* vertexData, u64 vertexCount, u64 vertexStride, bool autoupload)
	{
		VT_PROFILE_FUNCTION();

		u64 bytes = vertexCount * vertexStride;
		switch (Renderer::GetAPI())
		{
		case RenderAPI::API::None:    return nullptr;
		case RenderAPI::API::Vulkan:  return CreateRef<RHI::VulkanVertexBuffer>(vertexData, bytes, 0, autoupload);
		}
		VT_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}