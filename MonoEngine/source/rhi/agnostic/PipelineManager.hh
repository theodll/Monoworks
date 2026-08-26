#pragma once
#include <common/Base.hh>
#include <common/SafeQueue.hh>

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
		/// @brief Initializes the Pipeline Manager
		static void Init();

		/// @brief Shutdown the Pipeline Manager
		static void Shutdown();

		static void BatchCompile();

		/**
		 * @brief Creates or returns the graphics pipeline matching the given info.
		 * @param pInfo Pointer to the struct of which a graphics pipeline will be created or returned.
		 * @param Optional Pointer to the hash variable which is to be filled with the hash the pipeline is refereed to by the hash map.
		 */
		static std::expected<Ref<IGraphicsPipeline>, EResult>	CreateGraphicsPipeline( const GraphicsPipelineCreationInfo* pInfo, Hash::hash_t* MW_NULLABLE pHash = nullptr, bool deffered = true ) NOEXCEPT;
		
		/**
		 * @brief Creates or returns the compute pipeline matching the given info.
		 * @param pInfo Pointer to the struct of which a compute pipeline will be created or returned.
		 * @param Optional Pointer to the hash variable which is to be filled with the hash the pipeline is refereed to by the hash map.
		 */
		static std::expected<Ref<IComputePipeline>, EResult>	CreateComputePipeline( const ComputePipelineCreationInfo* pInfo, Hash::hash_t* MW_NULLABLE pHash = nullptr, bool deffered = true ) NOEXCEPT;

		/**
		 * @brief Removes the graphics pipeline from the graphics pipeline cache and invalidates the hash & reference.
		 * @param hash Hash of the graphics pipeline to delete.
		 */
		static void DeleteGraphicsPipeline( Hash::hash_t hash );

		/**
		 * @brief Removes the compute pipeline from the compute pipeline cache and invalidates the hash @ reference.
		 * @param hash Hash of the compute pipeline to delete.
		 */
		static void DeleteComputePipeline( Hash::hash_t hash );

		/// @brief Gets graphics pipeline by hash
		NODISCARD static std::expected<Ref<IGraphicsPipeline>, EResult> GetGraphicsPipelineByHash( Hash::hash_t hash ) NOEXCEPT;
		/// @brief Gets compute pipeline by hash
		NODISCARD static std::expected<Ref<IComputePipeline>, EResult>	GetComputePipelineByHash( Hash::hash_t hash ) NOEXCEPT;

		/// @brief Gets graphics pipeline by info 
		NODISCARD static std::expected<Ref<IGraphicsPipeline>, EResult> GetGraphicsPipelineByInfo( const GraphicsPipelineCreationInfo* pInfo ) NOEXCEPT;

		/// @brief Gets compute pipeline by info
		NODISCARD static std::expected<Ref<IComputePipeline>, EResult>	GetComputePipelineByInfo( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT;

		NODISCARD static u32 GetTotalPipelineCount() NOEXCEPT { return m_TotalPipelineCount; };
		NODISCARD static u32 GetTotalCompiledPipelineCount() NOEXCEPT { return m_TotalCompiledPipelineCount; };
		
		NODISCARD static u32 GetGraphicsPipelineCount() NOEXCEPT { return m_GraphicsPipelineCount; };
		NODISCARD static u32 GetCompiledGraphicsPipelineCount() NOEXCEPT { return m_CompiledGraphicsPipelineCount; };
		
		NODISCARD static u32 GetComputePipelineCount() NOEXCEPT { return m_ComputePipelineCount; };
		NODISCARD static u32 GetCompiledComputePipelineCount() NOEXCEPT { return m_CompiledComputePipelineCount; };


	private:
		static boost::unordered::unordered_map<Hash::hash_t, Ref<IGraphicsPipeline>> m_GraphicPipelineCache;
		static boost::unordered::unordered_map<Hash::hash_t, Ref<IComputePipeline>> m_ComputePipelineCache;

		// u8 component is 0 if graphics and 1 if compute
		static CSafeQueue<std::tuple<std::variant<GraphicsPipelineCreationInfo, ComputePipelineCreationInfo>, Hash::hash_t, u8>> m_PipelinesToCompile;

		static u32 m_TotalPipelineCount;
		static u32 m_TotalCompiledPipelineCount;
		
		static u32 m_GraphicsPipelineCount;
		static u32 m_ComputePipelineCount;
		
		static u32 m_CompiledGraphicsPipelineCount;
		static u32 m_CompiledComputePipelineCount;
	};
}
