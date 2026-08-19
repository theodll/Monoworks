#include <mwpch.hh>

#include "PipelineManager.hh"

namespace Monoworks::RHI 
{
	u32 CPipelineManager::m_CompiledComputePipelineCount;
	u32 CPipelineManager::m_CompiledGraphicsPipelineCount;
	u32 CPipelineManager::m_ComputePipelineCount;
	u32 CPipelineManager::m_GraphicsPipelineCount;
	u32 CPipelineManager::m_TotalCompiledPipelineCount;
	u32 CPipelineManager::m_TotalPipelineCount;
	boost::unordered::unordered_map<Hash::hash_t, Ref<IComputePipeline>> CPipelineManager::m_ComputePipelinesCache;
	boost::unordered::unordered_map<Hash::hash_t, Ref<IGraphicsPipeline>> CPipelineManager::m_GraphicPipelineCache;

	void CPipelineManager::Init()
	{
		MW_PROFILE_FUNC;

	}

	void CPipelineManager::Shutdown()
	{
		MW_PROFILE_FUNC;

	}

	Ref<IGraphicsPipeline> CPipelineManager::CreateGraphicsPipeline( const GraphicsPipelineCreationInfo* pInfo, Hash::hash_t* MW_NULLABLE pHash /*= nullptr */, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// TODO: thread safe
		const Hash::hash_t hash = GetGraphicsPipelineInfoHash( pInfo );

		if ( m_GraphicPipelineCache.contains( hash ) )
			return m_GraphicPipelineCache[hash];

		Ref<IGraphicsPipeline> p;
		auto defInfo = *pInfo;
		if ( deffered && !( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT ) )
		{
			MW_API_WARN( "Instructed CPipelineManager to deffer pipeline compilation/initalization without MW_PIPELINE_CREATION_FLAGS_DEFFERED_ININTALIZATION_BIT. Setting bit automatically" );
			defInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
			p = IGraphicsPipeline::Create( &defInfo );
		}
		else 
		{
			p = IGraphicsPipeline::Create( pInfo );
		}
		
		m_GraphicPipelineCache[hash] = p;
		m_TotalPipelineCount++;
		m_GraphicsPipelineCount++;

		if ( !deffered && !p->IsCompiled() )
		{
			m_TotalCompiledPipelineCount++;
			m_CompiledGraphicsPipelineCount++;
			try
			{ p->Invalidate( &defInfo ); }
			catch ( EResult r )
			{
				switch ( r )
				{
				case MW_ERROR_UNKNOWN || MW_ERROR_CACHE_INVALID:
					defInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_COMPILE_WIHTOUT_CACHE_BIT;
					break;
				default:
					break;
				};

				try { p->Invalidate( &defInfo ); }
				catch ( ... ) 
				{
					MW_ERROR("Discarding Pipeline as unable to compile.");
					m_TotalCompiledPipelineCount--;
					m_CompiledGraphicsPipelineCount--;
				};
				
			}
		}
		
		if ( pHash )
			*pHash = hash;

		return p;
	}

	Ref<IComputePipeline> CPipelineManager::CreateComputePipeline( const ComputePipelineCreationInfo* pInfo, Hash::hash_t* MW_NULLABLE pHash /*= nullptr */, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	Ref<IGraphicsPipeline> CPipelineManager::RecreateGraphicsPipeline( const GraphicsPipelineCreationInfo* pNewInfo, Hash::hash_t pipelineHash, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	Ref<IGraphicsPipeline> CPipelineManager::RecreateGraphicsPipeline( const GraphicsPipelineCreationInfo* pNewInfo, const GraphicsPipelineCreationInfo* pOldInfo, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	Ref<IComputePipeline> CPipelineManager::RecreateComputePipeline( const ComputePipelineCreationInfo* pNewInfo, Hash::hash_t pipelineHash, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	Ref<IComputePipeline> CPipelineManager::RecreateComputePipeline( const ComputePipelineCreationInfo* pNewInfo, const ComputePipelineCreationInfo* pOldInfo, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	NODISCARD Ref<IGraphicsPipeline> CPipelineManager::GetGraphicsPipelineByHash( Hash::hash_t hash ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	NODISCARD Ref<IComputePipeline> CPipelineManager::GetComputePipelineByHash( Hash::hash_t hash ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	NODISCARD Ref<IGraphicsPipeline> CPipelineManager::GetGraphicsPipelineByInfo( const GraphicsPipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}

	NODISCARD Ref<IComputePipeline> CPipelineManager::GetComputePipelineByInfo( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}
}
