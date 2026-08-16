#pragma once
#include <common/Base.hh>

#include "GraphicsPipeline.hh"

namespace Monoworks::RHI 
{
	struct ComputePipelineCreationInfo
	{
		void* Signature; // VkPipelineLayout / D3D12RootSignature
		EPipelineCreationFlags Flags; 

		SShaderObject ComputeShader;
	};

	class IComputePipeline 
	{
	public:
		virtual ~IComputePipeline() NOEXCEPT = default;

		virtual void Init( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT = 0;
		virtual void Shutdown() NOEXCEPT = 0;

		virtual void Invalidate( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT = 0;

		static Ref<IComputePipeline> Create( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT;
	
	};
}
