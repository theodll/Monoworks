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

	boost::unordered::unordered_map<Hash::hash_t, Ref<IComputePipeline>> CPipelineManager::m_ComputePipelineCache;
	boost::unordered::unordered_map<Hash::hash_t, Ref<IGraphicsPipeline>> CPipelineManager::m_GraphicPipelineCache;

	void CPipelineManager::Init()
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Initialize CPipelineManager" );
	}

	void CPipelineManager::Shutdown()
	{
		MW_PROFILE_FUNC;
		for ( auto& p : m_GraphicPipelineCache )
		{
			p.second->Shutdown();
		}
		for ( auto& p : m_ComputePipelineCache )
		{
			p.second->Shutdown();
		}
		m_GraphicPipelineCache.clear();
		m_ComputePipelineCache.clear();

		MW_INFO( "Shutdown CPipelineManager" );
	}

	std::expected<Ref<IGraphicsPipeline>, EResult> CPipelineManager::CreateGraphicsPipeline( const GraphicsPipelineCreationInfo* pInfo, Hash::hash_t* MW_NULLABLE pHash /*= nullptr */, bool deffered ) NOEXCEPT
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
			MW_API_WARN( "Instructed CPipelineManager to deffer pipeline compilation without MW_PIPELINE_CREATION_FLAGS_DEFFERED_ININTALIZATION_BIT. Setting bit automatically" );
			defInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
			p = IGraphicsPipeline::Create( &defInfo );
			m_PipelinesToCompile.Push( std::make_tuple( *pInfo, hash, 0 ) );
		}
		else 
		{
			m_TotalCompiledPipelineCount++;
			m_CompiledGraphicsPipelineCount++;
			try
			{
				p = IGraphicsPipeline::Create( pInfo );
			}
			catch ( EResult r )
			{
				if ( r == MW_ERROR_UNKNOWN || r == MW_ERROR_CACHE_INVALID )
					defInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_COMPILE_WIHTOUT_CACHE_BIT;

				try { p->Invalidate( &defInfo ); }
				catch ( ... )
				{
					MW_ERROR( "Discarding Pipeline at hash {}: Unable to compile.", hash );
					m_TotalCompiledPipelineCount--;
					m_CompiledGraphicsPipelineCount--;
				};
			}
		}
		
		m_GraphicPipelineCache[hash] = p;
		m_TotalPipelineCount++;
		m_GraphicsPipelineCount++;
		
		if ( pHash )
			*pHash = hash;

		return p;
	}

	std::expected<Ref<IComputePipeline>, EResult> CPipelineManager::CreateComputePipeline( const ComputePipelineCreationInfo * pInfo, Hash::hash_t* MW_NULLABLE pHash /*= nullptr */, bool deffered ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// TODO: thread safe
		const Hash::hash_t hash = GetComputePipelineInfoHash( pInfo );

		if ( m_ComputePipelineCache.contains( hash ) )
			return m_ComputePipelineCache[hash];

		Ref<IComputePipeline> p;
		auto defInfo = *pInfo;
		if ( deffered && !( pInfo->Flags & MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT ) )
		{
			MW_API_WARN( "Instructed CPipelineManager to deffer pipeline compilation without MW_PIPELINE_CREATION_FLAGS_DEFFERED_ININTALIZATION_BIT. Setting bit automatically" );
			defInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
			p = IComputePipeline::Create( &defInfo );
			m_PipelinesToCompile.Push( std::make_tuple( *pInfo, hash, 1));
		}
		else
		{
			m_TotalCompiledPipelineCount++;
			m_CompiledComputePipelineCount++;
			try
			{
				p = IComputePipeline::Create( pInfo );
			}
			catch ( EResult r )
			{
				if ( r == MW_ERROR_UNKNOWN || r == MW_ERROR_CACHE_INVALID )
					defInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_COMPILE_WIHTOUT_CACHE_BIT;

				try { p->Invalidate( &defInfo ); }
				catch ( ... )
				{
					MW_ERROR( "Discarding Pipeline at hash {}: Unable to compile.", hash );
					m_TotalCompiledPipelineCount--;
					m_CompiledComputePipelineCount--;
				};
			}
		}

		m_ComputePipelineCache[hash] = p;
		m_TotalPipelineCount++;
		m_ComputePipelineCount++;

		if ( pHash )
			*pHash = hash;

		return p;
	}

	NODISCARD std::expected<Ref<IGraphicsPipeline>, EResult> CPipelineManager::GetGraphicsPipelineByHash(Hash::hash_t hash) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( m_GraphicPipelineCache.contains(hash) )
		{
			return m_GraphicPipelineCache[hash];
		} 
		else 
		{
			return std::unexpected( MW_ERROR_NON_EXISTANT );
		}
	}

	NODISCARD std::expected<Ref<IComputePipeline>, EResult> CPipelineManager::GetComputePipelineByHash( Hash::hash_t hash ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( m_ComputePipelineCache.contains( hash ) )
		{
			return m_ComputePipelineCache[hash];
		}
		else
		{
			return std::unexpected( MW_ERROR_NON_EXISTANT );
		}

	}

	NODISCARD std::expected<Ref<IGraphicsPipeline>, EResult> CPipelineManager::GetGraphicsPipelineByInfo( const GraphicsPipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		const Hash::hash_t hash = GetGraphicsPipelineInfoHash( pInfo );
		if ( m_GraphicPipelineCache.contains( hash ) )
		{
			return m_GraphicPipelineCache[hash];
		}
		else 
		{
			return std::unexpected( MW_ERROR_NON_EXISTANT );
		}
	}

	NODISCARD std::expected<Ref<IComputePipeline>, EResult> CPipelineManager::GetComputePipelineByInfo( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT
	{
		MW_PROFILE_FUNC;		
		const Hash::hash_t hash = GetComputePipelineInfoHash( pInfo );
		if ( m_ComputePipelineCache.contains( hash ) )
		{
			return m_ComputePipelineCache[hash];
		}
		else
		{
			return std::unexpected( MW_ERROR_NON_EXISTANT );
		}
	}

	void CPipelineManager::BatchCompile()
	{
		MW_PROFILE_FUNC; 

		// TODO: real batch compilation with actual use the batch compilation property of vkCreateGraphicsPipelines / vkCreateComputePipelines.

		while ( !m_PipelinesToCompile.IsEmpty() )
		{
			auto tuple = m_PipelinesToCompile.Front();

			auto hash = std::get<Hash::hash_t>( tuple );
			auto infoUnion = std::get<std::variant<GraphicsPipelineCreationInfo, ComputePipelineCreationInfo>>( tuple );
			
			// Graphics Pipelines
			if ( std::get<u8>( tuple ) == 0 )
			{
				auto p = GetGraphicsPipelineByHash( hash );
				
				if ( p )
				{
					auto graphicsInfo = std::get<GraphicsPipelineCreationInfo>( infoUnion );
					try
					{
						m_TotalCompiledPipelineCount++;
						m_CompiledGraphicsPipelineCount++;
						p.value()->Invalidate( &graphicsInfo );
					}
					catch ( EResult r )
					{
						if ( r == MW_ERROR_UNKNOWN || r == MW_ERROR_CACHE_INVALID )
							graphicsInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_COMPILE_WIHTOUT_CACHE_BIT;

						try { p.value()->Invalidate( &graphicsInfo ); }
						catch ( ... )
						{
							MW_ERROR( "Discarding Pipeline at hash {}: Unable to compile.", hash );
							m_TotalCompiledPipelineCount--;
							m_CompiledGraphicsPipelineCount--;
						};
					}
				} 
				else 
				{
					MW_API_WARN( "Non-existant hash ({}) of Graphics Pipeline referenced in deffered-compilation pipeline queue. Discarding.", hash );
				}				
			} 
			else if ( std::get<u8>( tuple ) == 1 ) // Compute Pipelines
			{
				auto p = GetComputePipelineByHash( hash );

				if ( p )
				{
					auto computeInfo = std::get<ComputePipelineCreationInfo>( infoUnion );
					try
					{
						p.value()->Invalidate( &computeInfo );
					}
					catch ( EResult r )
					{
						if ( r == MW_ERROR_UNKNOWN || r == MW_ERROR_CACHE_INVALID )
							computeInfo.Flags |= MW_PIPELINE_CREATION_FLAGS_COMPILE_WIHTOUT_CACHE_BIT;

						try { p.value()->Invalidate( &computeInfo ); }
						catch ( ... )
						{
							MW_ERROR( "Discarding Pipeline at hash {}: Unable to compile.", hash );
							m_TotalCompiledPipelineCount--;
							m_CompiledComputePipelineCount--;
						};
					}
				}
				else
				{
					MW_API_WARN( "Non-existant hash ({}) of Graphics Pipeline referenced in deffered-compilation pipeline queue. Discarding.", hash );
				}
			}
			else 
			{
				MW_API_WARN( "Invalid type of Pipeline passed to deffered-compilation pipeline queue. Discarding." );
			}

		}

	}

}
