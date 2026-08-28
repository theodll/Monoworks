#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/IndexBuffer.hh>

namespace Monoworks 
{
	class ISceneGraph 
	{
	public:
		virtual ~ISceneGraph();

		virtual void MW_NOTHROW AddGeometry( Ref<RHI::IVertexBuffer> hVertexBuffer, Ref<RHI::IIndexBuffer> hIndexBuffer);
	};
}
