#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/IndexBuffer.hh>
#include <renderer/Material.h>

namespace Monoworks 
{
	// temporary thing
	class CMesh 
	{
	public:
		Ref<RHI::IVertexBuffer> VertexBuffer;
		Ref<RHI::IIndexBuffer> IndexBuffer;
		Ref<CMaterial> Material;
	};
}

