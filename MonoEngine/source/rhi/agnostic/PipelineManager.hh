#pragma once
#include <common/Base.hh>

#include "GraphicsPipeline.hh"
#include "ComputePipeline.hh"

#include <boost/unordered/unordered_map.hpp>
#include <tuple>

namespace Monoworks::RHI 
{
	constexpr inline static Hash::hash_t GetGraphicsPipelineInfoHash( const GraphicsPipelineCreationInfo* pInfo)
	{
		MW_PROFILE_FUNC;
		Hash::hash_t hash;

		Hash::HashCombine( hash, pInfo->Flags );
		Hash::HashCombine( hash, pInfo->DepthAttachmentFormat );
		Hash::HashCombine( hash, pInfo->ViewportCount );
		Hash::HashCombine( hash, pInfo->ScissorCount );
		Hash::HashCombine( hash, pInfo->StencilAttachmentFormat );
		Hash::HashCombine( hash, pInfo->CompareOp );
		Hash::HashCombine( hash, pInfo->CullMode );
		Hash::HashCombine( hash, pInfo->Topology );
		Hash::HashCombine( hash, pInfo->PolygonMode );

		Hash::HashCombine( hash, Hash::HashVector(pInfo->ColorBlendAttachments) );
		Hash::HashCombine( hash, Hash::HashVector( pInfo->DynamicStates ) );
		Hash::HashCombine( hash, Hash::HashVector( pInfo->ColorFormats ) );

		for ( const auto& shaderObjects  : pInfo->ShaderObjects )
		{
			Hash::HashCombine( hash, Hash::FastHashBytes( shaderObjects.pEntrypoint, std::strlen( shaderObjects.pEntrypoint ) ) );
			Hash::HashCombine( hash, shaderObjects.ShaderStage );
			
			Hash::HashCombine( hash, Hash::FastHashBytes( shaderObjects.Code.pCode, shaderObjects.Code.Size ) );
		}

		Hash::HashCombine( hash, pInfo->VertexLayout.GetHash() );

		return hash;

	};

	constexpr inline static Hash::hash_t GetComputePipelineInfoHash( const ComputePipelineCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		Hash::hash_t hash;

		Hash::HashCombine( hash, pInfo->Flags );

		Hash::HashCombine( hash, pInfo->ComputeShader.ShaderStage );
		Hash::HashCombine( hash, Hash::FastHashBytes( pInfo->ComputeShader.Code.pCode, pInfo->ComputeShader.Code.Size ) );
		Hash::HashCombine( hash, Hash::FastHashBytes( pInfo->ComputeShader.pEntrypoint, pInfo->ComputeShader.Code.Size ) );
		return hash;
	}

	class CPipelineManager 
	{
	public:
		static void Init();
		static void Shutdown();

		static Ref<IGraphicsPipeline> CreateGraphicsPipeline( const GraphicsPipelineCreationInfo* pInfo, Hash::hash_t* MW_NULLABLE pHash = nullptr );
		static Ref<IComputePipeline> CreateComputePipeline( const ComputePipelineCreationInfo* pInfo );

	private:
		boost::unordered::unordered_map<Hash::hash_t, Ref<IGraphicsPipeline>> m_GraphicPipelines;
		boost::unordered::unordered_map<Hash::hash_t, Ref<IComputePipeline>> m_ComputePipelines;
	};
}
