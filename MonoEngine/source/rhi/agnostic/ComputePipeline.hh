#pragma once
#include <common/Base.hh>

#include "GraphicsPipeline.hh"

namespace Monoworks::RHI 
{
	struct ComputePipelineCreationInfo
	{
		PipelineSignature Signature; // VkPipelineLayout / D3D12RootSignature
		EPipelineCreationFlags Flags; 

		SShaderObject ComputeShader;
	};

	class IComputePipeline 
	{
	public:
		virtual ~IComputePipeline() NOEXCEPT = default;

		virtual void Init( const ComputePipelineCreationInfo* pInfo ) = 0;
		virtual void Shutdown() NOEXCEPT = 0;

		virtual EResult Invalidate( const ComputePipelineCreationInfo* pInfo ) = 0;

		virtual bool IsCompiled() = 0;

		NODISCARD virtual PipelineSignature* GetSignature() NOEXCEPT = 0;

		static Ref<IComputePipeline> Create( const ComputePipelineCreationInfo* pInfo );
	
	};
}
