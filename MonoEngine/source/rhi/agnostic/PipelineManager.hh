#pragma once
#include <common/Base.hh>

#include "GraphicsPipeline.hh"
#include "ComputePipeline.hh"

#include <boost/unordered/unordered_map.hpp>

namespace Monoworks::RHI 
{
	namespace Monoworks::RHI
	{

		struct GraphicsPipelineCreationInfoHasher
		{
			u64 operator()( const GraphicsPipelineCreationInfo& rInfo ) const noexcept
			{
				u64 seed = 0;

				// 1. Primitive Typen & Enums
				HashCombine( seed, static_cast< u64 >( rInfo.Flags ) );
				HashCombine( seed, static_cast< u64 >( rInfo.DepthAttachmentFormat ) );
				HashCombine( seed, static_cast< u64 >( rInfo.StencilAttachmentFormat ) );
				HashCombine( seed, static_cast< u64 >( rInfo.ViewportCount ) );
				HashCombine( seed, static_cast< u64 >( rInfo.ScissorCount ) );
				HashCombine( seed, static_cast< u64 >( rInfo.CompareOp ) );
				HashCombine( seed, static_cast< u64 >( rInfo.CullMode ) );
				HashCombine( seed, static_cast< u64 >( rInfo.Topology ) );
				HashCombine( seed, static_cast< u64 >( rInfo.PolygonMode ) );

				HashCombine( seed, rInfo.VertexLayout.GetHash() );

				HashCombine( seed, HashVector( rInfo.ColorFormats ) );
				HashCombine( seed, HashVector( rInfo.DynamicStates ) );

				u64 blendSeed = rInfo.ColorBlendAttachments.size();
				for ( const auto& rAttachment : rInfo.ColorBlendAttachments )
				{
					u64 attachmentHash = static_cast< u64 >( rAttachment.BlendMode );
					HashCombine( attachmentHash, static_cast< u64 >( rAttachment.BlendEnable ) );
					HashCombine( blendSeed, attachmentHash );
				}
				HashCombine( seed, blendSeed );

				u64 shaderSeed = rInfo.ShaderObjects.size();
				for ( const auto& rShader : rInfo.ShaderObjects )
				{
					u64 singleShaderHash = static_cast< u64 >( rShader.ShaderStage );

					if ( rShader.pEntrypoint )
					{
						const std::string_view entryView( rShader.pEntrypoint );
						HashCombine( singleShaderHash, FastHashBytes( entryView.data(), entryView.size() ) );
					}

					if ( rShader.Code.pCode && rShader.Code.Size > 0 )
					{
						HashCombine( singleShaderHash, FastHashBytes( rShader.Code.pCode, rShader.Code.Size ) );
					}

					HashCombine( shaderSeed, singleShaderHash );
				}
				HashCombine( seed, shaderSeed );
				HashCombine( seed, reinterpret_cast< uintptr_t >( rInfo.Signature ) );

				return seed;
			}
		};
	}

	class CPipelineManager 
	{
	public:
		static void Init();
		static void Shutdown();

	private:
		// TODO: UUID  
		boost::unordered::unordered_map<GraphicsPipelineCreationInfo, Ref<IGraphicsPipeline>> m_GraphicPipelines;
		boost::unordered::unordered_map<ComputePipelineCreationInfo, Ref<IComputePipeline>> m_ComputePipelines;
	};
}
