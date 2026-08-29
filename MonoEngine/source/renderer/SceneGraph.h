#pragma once
#include <common/Base.hh>

#include <renderer/Material.h>

#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/IndexBuffer.hh>

namespace Monoworks 
{
	struct ModelPushConstant 
	{
		Matrix Transform;
		u32 EntityID = UINT32_MAX; // TODO: Implement
		u32 MaterialMask = UINT32_MAX; // TODO: Implement
	};

	class ISceneGraph 
	{
	public:
		virtual ~ISceneGraph();

		// TODO: Replace with model
		virtual void MW_NOTHROW AddGeometry( Ref<RHI::IVertexBuffer> hVertexBuffer, Ref<RHI::IIndexBuffer> hIndexBuffer, Ref<CMaterial> hMaterial, Matrix transform ) NOEXCEPT; 

	};
}
