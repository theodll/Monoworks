#include <common/Base.hh>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/PipelineManager.hh>

#include "StaticRenderer.hh"
#include "FrameGraph.hh"

namespace Monoworks 
{
	using namespace RHI;
	constexpr u32 GlobalScopeSignature = 0;

	NODISCARD static std::expected<std::pair<u32, u32>, EResult> FindParameterBlockAndBindingNumberByString( std::string_view parameterBlockName, std::string_view bindingName, slang::ProgramLayout* pLayout )
	{
		MW_PROFILE_FUNC;

		slang::VariableLayoutReflection* pBlockVar = nullptr;

		for ( u32 i = 0; i < pLayout->getParameterCount(); ++i )
		{
			slang::VariableLayoutReflection* pParam = pLayout->getParameterByIndex( i );
			if ( parameterBlockName == pParam->getName() )
			{
				pBlockVar = pParam;
				break;
			}
		}

		if ( pBlockVar == nullptr )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		slang::TypeLayoutReflection* pBlockTypeLayout = pBlockVar->getTypeLayout();

		if ( pBlockTypeLayout->getKind() != slang::TypeReflection::Kind::ParameterBlock )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		const u32 setIndex = static_cast< u32 >( pBlockVar->getOffset( slang::ParameterCategory::SubElementRegisterSpace ) );

		slang::VariableLayoutReflection* pElementVar = pBlockTypeLayout->getElementVarLayout();
		slang::TypeLayoutReflection* pElementTypeLayout = pElementVar->getTypeLayout();

		const u32 containerBindingOffset = static_cast< u32 >( pElementVar->getOffset( slang::ParameterCategory::DescriptorTableSlot ) );

		for ( u32 i = 0; i < pElementTypeLayout->getFieldCount(); ++i )
		{
			slang::VariableLayoutReflection* pField = pElementTypeLayout->getFieldByIndex( i );
			if ( bindingName == pField->getName() )
			{
				const u32 bindingIndex = containerBindingOffset + static_cast< u32 >( pField->getOffset( slang::ParameterCategory::DescriptorTableSlot ) );
				return std::make_pair( setIndex, bindingIndex );
			}
		}

		return std::unexpected( MW_ERROR_NON_EXISTANT );

	}

	NODISCARD static std::expected<u32, EResult> FindBindingNumberByString( std::string_view name, slang::ProgramLayout* pLayout ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		slang::VariableLayoutReflection* globals = pLayout->getGlobalParamsVarLayout();
		slang::TypeLayoutReflection* globalsType = globals->getTypeLayout();

		const auto cstr = name.data();
		const SlangInt fieldIndex = globalsType->findFieldIndexByName( cstr );

		if ( fieldIndex < 0 )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		slang::VariableLayoutReflection* field = globalsType->getFieldByIndex( fieldIndex );
		const size_t binding = field->getOffset( slang::ParameterCategory::DescriptorTableSlot );

		return binding;

	}


	CComputePrePass::CComputePrePass( const ComputePrePassCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		MW_PROFILE_FUNC;

		if ( !pInfo->hShader )
		{
			MW_API_ERROR( "Invalid Shader Reference Passed" );
			throw std::runtime_error( "Invalid Shader Reference Passed" );
			return;
		};

		m_hShader = pInfo->hShader;

		const auto reflectionData = m_hShader->ReflectOnShader();
		auto entrypoints = m_hShader->GetShaderEntrypoints();
		const char* entrypoint = entrypoints[MW_SHADER_STAGE_COMPUTE].c_str();

		Slang::ComPtr<slang::IBlob> code;
		Slang::ComPtr<slang::IBlob> diagnostics;

		const SlangInt entrypointPointIndex = 0;
		const SlangInt targetIndex = 0;

		const SlangResult result =
			m_hShader->GetShaderProgram()->getEntryPointCode(
				entrypointPointIndex,
				targetIndex,
				code.writeRef(),
				diagnostics.writeRef() );


		if ( SLANG_FAILED( result ) )
		{
			if ( diagnostics )
				MW_ERROR( "Failed to get entry point code for Compute Pre Pass pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

			return;
		}

		SShaderObject computeShader;
		computeShader.pEntrypoint = entrypoint;
		computeShader.ShaderStage = MW_SHADER_STAGE_COMPUTE;
		computeShader.Code = { code->getBufferPointer(), code->getBufferSize() };


		RHI::ComputePipelineCreationInfo pipelineInfo{};
		pipelineInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
		pipelineInfo.Signature = reflectionData.pPipelineSignature;
		pipelineInfo.ComputeShader = computeShader;

		std::expected<Ref<IComputePipeline>, EResult> pipeline;
		try
		{
			pipeline = CPipelineManager::CreateComputePipeline( &pipelineInfo, &m_PipelineHash );
		}
		catch ( ... )
		{
			MW_WARN( "Failed to create Compute Pipeline for Post Processing Pass " );
		}

		if ( pipeline )
			m_hComputePipeline = pipeline.value();

		for ( auto i{ 0uz }; i < MFIF; i++ )
			m_pDescriptors[i] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[GlobalScopeSignature] );


	}

	MW_NOTHROW CComputePrePass::~CComputePrePass() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CPipelineManager::DeleteComputePipeline( m_PipelineHash );
		m_hComputePipeline = nullptr;
	}

	void CComputePrePass::BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteImage( m_pDescriptors[i], binding.value(), hTexture );
				m_BindingsWritten[binding.value()] = true;
			}
	}

	void CComputePrePass::BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteSampler( m_pDescriptors[i], binding.value(), hSampler );
				m_BindingsWritten[binding.value()] = true;
			}
	}

	void CComputePrePass::BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}

	
	CGraphicsPrePass::CGraphicsPrePass( const GraphicsPrePassCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
	}

	MW_NOTHROW CGraphicsPrePass::~CGraphicsPrePass() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CPipelineManager::DeleteComputePipeline( m_PipelineHash );
		m_hGraphicsPipeline = nullptr;
	}

	void CGraphicsPrePass::BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteImage( m_pDescriptors[i], binding.value(), hTexture );
				m_BindingsWritten[binding.value()] = true;
			}
	}

	void CGraphicsPrePass::BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteSampler( m_pDescriptors[i], binding.value(), hSampler );
				m_BindingsWritten[binding.value()] = true;
			}
	}

	void CGraphicsPrePass::BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;


		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[i], binding.value(), hUniformBuffer );
				m_BindingsWritten[binding.value()] = true;
			}
	}

	void CGraphicsPrePass::RegisterExecutionScopeCallback( const std::function<void>& rpExecutionScopeCallback )
	{
		MW_PROFILE_FUNC;
	}

	CPostProcessPass::CPostProcessPass( const PostProcessPassCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		
		if ( !pInfo->hShader )
		{
			MW_API_ERROR( "Invalid Shader Reference Passed" );
			throw std::runtime_error( "Invalid Shader Reference Passed" );
			return;
		};
		
		m_hShader = pInfo->hShader;

		const auto reflectionData = m_hShader->ReflectOnShader();
		auto entrypoints = m_hShader->GetShaderEntrypoints();
		const char* entrypoint = entrypoints[MW_SHADER_STAGE_COMPUTE].c_str();

		Slang::ComPtr<slang::IBlob> code;
		Slang::ComPtr<slang::IBlob> diagnostics;

		const SlangInt entrypointPointIndex = 0;
		const SlangInt targetIndex = 0;

		const SlangResult result = 
			m_hShader->GetShaderProgram()->getEntryPointCode(
			entrypointPointIndex,
			targetIndex,
			code.writeRef(),
			diagnostics.writeRef() );


		if ( SLANG_FAILED( result ) )
		{
			if ( diagnostics )
				MW_ERROR( "Failed to get entry point code for Post Processing pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );
			
			return;
		}

		SShaderObject computeShader;
		computeShader.pEntrypoint = entrypoint;
		computeShader.ShaderStage = MW_SHADER_STAGE_COMPUTE;
		computeShader.Code = { code->getBufferPointer(), code->getBufferSize() };


		RHI::ComputePipelineCreationInfo pipelineInfo{};
		pipelineInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
		pipelineInfo.Signature = reflectionData.pPipelineSignature;
		pipelineInfo.ComputeShader = computeShader;

		std::expected<Ref<IComputePipeline>, EResult> pipeline;
		try
		{
			pipeline = CPipelineManager::CreateComputePipeline( &pipelineInfo, &m_PipelineHash );
		}
		catch ( ... )
		{
			MW_WARN( "Failed to create Compute Pipeline for Post Processing Pass " );
		}

		if ( pipeline )
			m_hComputePipeline = pipeline.value();
		
		for ( auto i{ 0uz }; i < MFIF; i++ )
			m_pDescriptors[i] = CDescriptorManager::Allocate(reflectionData.pDescriptorSignatures[GlobalScopeSignature]);


	};

	MW_NOTHROW CPostProcessPass::~CPostProcessPass() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		CPipelineManager::DeleteComputePipeline( m_PipelineHash );

	};

	void CPostProcessPass::BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite )
	{
		MW_PROFILE_FUNC;
		
		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteImage( m_pDescriptors[i], binding.value(), hTexture );
				m_BindingsWritten[binding.value()] = true;
			}
	};

	void CPostProcessPass::BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteSampler( m_pDescriptors[i], binding.value(), hSampler );
				m_BindingsWritten[binding.value()] = true;
			}
	}

	void CPostProcessPass::BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite )
	{
		MW_PROFILE_FUNC;


		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[binding.value()] )
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[i], binding.value(), hUniformBuffer );
				m_BindingsWritten[binding.value()] = true;
			}

	};


	CDefferedResolutionPass::CDefferedResolutionPass( const DefferedResolutionPassCreateionInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		MW_PROFILE_FUNC;

		if ( !pInfo->hShader )
		{
			MW_API_ERROR( "Invalid Shader Reference Passed" );
			throw std::runtime_error( "Invalid Shader Reference Passed" );
			return;
		};

		m_hShader = pInfo->hShader;

		const auto reflectionData = m_hShader->ReflectOnShader();
		auto entrypoints = m_hShader->GetShaderEntrypoints();
		const char* entrypoint = entrypoints[MW_SHADER_STAGE_COMPUTE].c_str();

		Slang::ComPtr<slang::IBlob> code;
		Slang::ComPtr<slang::IBlob> diagnostics;

		const SlangInt entrypointPointIndex = 0;
		const SlangInt targetIndex = 0;

		const SlangResult result =
			m_hShader->GetShaderProgram()->getEntryPointCode(
				entrypointPointIndex,
				targetIndex,
				code.writeRef(),
				diagnostics.writeRef() );


		if ( SLANG_FAILED( result ) )
		{
			if ( diagnostics )
				MW_ERROR( "Failed to get entry point code for deffered resolution pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

			return;
		}

		SShaderObject computeShader;
		computeShader.pEntrypoint = entrypoint;
		computeShader.ShaderStage = MW_SHADER_STAGE_COMPUTE;
		computeShader.Code = { code->getBufferPointer(), code->getBufferSize() };


		RHI::ComputePipelineCreationInfo pipelineInfo{};
		pipelineInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
		pipelineInfo.Signature = reflectionData.pPipelineSignature;
		pipelineInfo.ComputeShader = computeShader;

		std::expected<Ref<IComputePipeline>, EResult> pipeline;
		try
		{
			pipeline = CPipelineManager::CreateComputePipeline( &pipelineInfo, &m_PipelineHash );
		}
		catch ( ... )
		{
			MW_WARN( "Failed to create Compute Pipeline for Deffered Resolution Pass " );
		}

		if ( pipeline )
			m_hComputePipeline = pipeline.value();

		std::array<RHI::DescriptorHandle, MFIF> globalScopeDescriptorSets{};
		
		for ( auto i{ 0uz }; i < MFIF; i++ )
			globalScopeDescriptorSets[i] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[GlobalScopeSignature] );

		if ( m_pDescriptors.size() < 1 )
			m_pDescriptors.resize( 1 );

		m_pDescriptors[GlobalScopeSignature] = globalScopeDescriptorSets;
	}


	CDefferedResolutionPass::~CDefferedResolutionPass() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// TODO: Implement destructor
	}


	void CDefferedResolutionPass::BindTexture( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( !hTexture )
		{
			MW_API_ERROR( "Passed invalid Uniform Buffer reference." );
			return;
		}

		if ( !bindings && bindings.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Parameter Block {} or Descriptor Slot {}: Non existant.", parameterBlockName.data(), bindingName.data() );
			return;
		}
		else if ( !bindings )
		{
			MW_API_WARN( "Failed to find binding for Parameter Block {} or Descriptor Slot {}: Unkown error.", parameterBlockName.data(), bindingName.data() );
			return;
		}

		auto [parameterBlock, descriptorSlot] = bindings.value();

		if ( m_pDescriptors.size() < parameterBlock )
			m_pDescriptors.resize( parameterBlock );

		if ( m_pDescriptors[parameterBlock][CStaticRenderer::GetCurrentFrameIndex()] == nullptr )
		{
			auto reflectionData = m_hShader->ReflectOnShader();
			if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
			{
				MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
				return;
			}

			for ( auto& frameDescriptors : m_pDescriptors )
				for ( auto i{ 0uz }; i < MFIF; i++ )
					frameDescriptors[i] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
		}

		if ( forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}] )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
				CDescriptorManager::WriteImage( m_pDescriptors[parameterBlock][i], descriptorSlot, hTexture );

			m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
		}

	}


	void CDefferedResolutionPass::BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[{GlobalScopeSignature, binding.value()}] )
		{

			for ( auto i{ 0uz }; i < MFIF; i++ )
				CDescriptorManager::WriteImage( m_pDescriptors[GlobalScopeSignature][i], binding.value(), hTexture );

			m_BindingsWritten[{GlobalScopeSignature, binding.value()}] = true;
			
		}
	}


	void CDefferedResolutionPass::BindSampler( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( !hSampler )
		{
			MW_API_ERROR( "Passed invalid Uniform Buffer reference." );
			return;
		}

		if ( !bindings && bindings.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Parameter Block {} or Descriptor Slot {}: Non existant.", parameterBlockName.data(), bindingName.data() );
			return;
		}
		else if ( !bindings )
		{
			MW_API_WARN( "Failed to find binding for Parameter Block {} or Descriptor Slot {}: Unkown error.", parameterBlockName.data(), bindingName.data() );
			return;
		}

		auto [parameterBlock, descriptorSlot] = bindings.value();

		if ( m_pDescriptors.size() < parameterBlock )
			m_pDescriptors.resize( parameterBlock );

		if ( m_pDescriptors[parameterBlock][CStaticRenderer::GetCurrentFrameIndex()] == nullptr )
		{
			auto reflectionData = m_hShader->ReflectOnShader();
			if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
			{
				MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
				return;
			}

			for ( auto& frameDescriptors : m_pDescriptors )
				for ( auto i{ 0uz }; i < MFIF; i++ )
					frameDescriptors[i] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
		}

		if ( forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}] )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
				CDescriptorManager::WriteSampler( m_pDescriptors[parameterBlock][i], descriptorSlot, hSampler );

			m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
		}
	}


	void CDefferedResolutionPass::BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}
		
		if ( forceRewrite || !m_BindingsWritten[{GlobalScopeSignature, binding.value()}] )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
				CDescriptorManager::WriteSampler( m_pDescriptors[GlobalScopeSignature][i], binding.value(), hSampler );
			
			m_BindingsWritten[{GlobalScopeSignature, binding.value()}] = true;
			
		}
	}


	void CDefferedResolutionPass::BindUBO( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( !hUniformBuffer )
		{
			MW_API_ERROR( "Passed invalid Uniform Buffer reference." );
			return;
		}

		if ( !bindings && bindings.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Parameter Block {} or Descriptor Slot {}: Non existant.", parameterBlockName.data(), bindingName.data() );
			return;
		}
		else if ( !bindings )
		{
			MW_API_WARN( "Failed to find binding for Parameter Block {} or Descriptor Slot {}: Unkown error.", parameterBlockName.data(), bindingName.data() );
			return;
		}

		auto [parameterBlock, descriptorSlot] = bindings.value();

		if ( m_pDescriptors.size() < parameterBlock )
			m_pDescriptors.resize( parameterBlock );

		if ( m_pDescriptors[parameterBlock][CStaticRenderer::GetCurrentFrameIndex()] == nullptr )
		{
			auto reflectionData = m_hShader->ReflectOnShader();
			if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
			{
				MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
				return;
			}

			for ( auto& frameDescriptors : m_pDescriptors )
				for ( auto i{ 0uz }; i < MFIF; i++ )
					frameDescriptors[i] = CDescriptorManager::Allocate(reflectionData.pDescriptorSignatures[parameterBlock]);	
		}

		if ( forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}] )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[parameterBlock][i], descriptorSlot, hUniformBuffer );

			m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
		}
		

	}

	void CDefferedResolutionPass::BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
		auto binding = FindBindingNumberByString( bindingName, m_hShader->GetShaderProgram()->getLayout() );
		if ( !binding && binding.error() == MW_ERROR_NON_EXISTANT )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Non existant.", bindingName.data() );
			return;
		}
		else if ( !binding )
		{
			MW_API_WARN( "Failed to find binding for Descriptor Slot {}: Unkown error.", bindingName.data() );
			return;
		}

		if ( forceRewrite || !m_BindingsWritten[{GlobalScopeSignature, binding.value()}] )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[GlobalScopeSignature][i], binding.value(), hUniformBuffer );

			m_BindingsWritten[{GlobalScopeSignature, binding.value()}] = true;
		}
			
	}

	CDefferedFrameGraph::CDefferedFrameGraph()	 NOEXCEPT 
	{
		MW_PROFILE_FUNC;
	};

	CDefferedFrameGraph::~CDefferedFrameGraph()	 NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_hComputePrePasses.clear();
		m_hGraphicsPrePasses.clear();
		m_hDefferedResolutionPasses.clear();
		m_hPostProcessPasses.clear();
	};

	void CDefferedFrameGraph::ExecutePrePasses() NOEXCEPT 
	{
		MW_PROFILE_FUNC;
	};
	
	void CDefferedFrameGraph::ExecuteCorePasses() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	};

	void CDefferedFrameGraph::ExecutePostPasses() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	};



	void CDefferedFrameGraph::AddPrePass( Ref<CComputePrePass> hComputePrePass, u32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( !hComputePrePass )
		{
			MW_API_WARN( "Passed invalid hComputePrePass to CDefferedFrameGraph::AddPrePass. Discarding.");
			return;
		}

		if ( executionPriority == UINT32_MAX )
		{
			m_hComputePrePasses.push_back( std::move( hComputePrePass ) );
			MW_INFO( "Register Compute Pre-Pass {} at execution priority {}", hComputePrePass.raw(), m_hComputePrePasses.size() );
		}
		else 
		{
			if ( m_hComputePrePasses.size() <= executionPriority + 1 )
			{
				m_hComputePrePasses.push_back( std::move( hComputePrePass ) );
				MW_INFO( "Register Compute Pre-Pass {} at execution priority {}", hComputePrePass.raw(), m_hComputePrePasses.size() );
			}
			else
			{
				m_hComputePrePasses.insert( m_hComputePrePasses.begin() + executionPriority, std::move( hComputePrePass ) );
				MW_INFO( "Register Compute Pre-Pass {} by inserting it at execution priority {}", hComputePrePass.raw(), executionPriority );
			}
		}
	};

	void CDefferedFrameGraph::AddPrePass( Ref<CGraphicsPrePass> hGraphicsPrePass, u32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( !hGraphicsPrePass )
		{
			MW_API_WARN( "Passed invalid hGraphicsPrePass to CDefferedFrameGraph::AddPrePass. Discarding." );
			return;
		}

		if ( executionPriority == UINT32_MAX )
		{
			m_hGraphicsPrePasses.push_back( std::move( hGraphicsPrePass ) );
			MW_INFO( "Register Graphics Pre-Pass {} at execution priority {}", hGraphicsPrePass.raw(), m_hGraphicsPrePasses.size() );
		}
		else
		{
			if ( m_hGraphicsPrePasses.size() <= executionPriority + 1 )
			{
				m_hGraphicsPrePasses.push_back( std::move( hGraphicsPrePass ) );
				MW_INFO( "Register Graphics Pre-Pass {} at execution priority {}", hGraphicsPrePass.raw(), m_hGraphicsPrePasses.size() );
			}
			else
			{
				m_hGraphicsPrePasses.insert( m_hGraphicsPrePasses.begin() + executionPriority, std::move( hGraphicsPrePass ) );
				MW_INFO( "Register Graphics Pre-Pass {} by inserting it at execution priority {}", hGraphicsPrePass.raw(), executionPriority );
			}
		}
	};

	void CDefferedFrameGraph::AddDefferedResolutionPass( Ref<CDefferedResolutionPass> hComputePass, u32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hComputePass )
		{
			MW_API_WARN( "Passed invalid hComputePass to CDefferedFrameGraph::AddDefferedResolutionPass. Discarding." );
			return;
		}

		if ( executionPriority == UINT32_MAX )
		{
			m_hDefferedResolutionPasses.push_back( std::move( hComputePass ) );
			MW_INFO( "Register Deffered Resolotion {} at execution priority {}", hComputePass.raw(), m_hDefferedResolutionPasses.size() );
		}
		else
		{
			if ( m_hGraphicsPrePasses.size() <= executionPriority + 1 )
			{
				m_hDefferedResolutionPasses.push_back( std::move( hComputePass ) );
				MW_INFO( "Register Deffered Resolotion Pre-Pass {} at execution priority {}", hComputePass.raw(), m_hDefferedResolutionPasses.size() );
			}
			else
			{
				m_hDefferedResolutionPasses.insert( m_hDefferedResolutionPasses.begin() + executionPriority, std::move( hComputePass ) );
				MW_INFO( "Register Deffered Resolotion Pre-Pass {} by inserting it at execution priority {}", hComputePass.raw(), executionPriority );
			}
		}
	};

	void CDefferedFrameGraph::AddPostProcessPass( Ref<CPostProcessPass> hPostProcessPass, u32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hPostProcessPass )
		{
			MW_API_WARN( "Passed invalid hPostProcessPass to CDefferedFrameGraph::AddDefferedResolutionPass. Discarding." );
			return;
		}

		if ( executionPriority == UINT32_MAX )
		{
			m_hPostProcessPasses.push_back( std::move( hPostProcessPass ) );
			MW_INFO( "Register Deffered Resolution {} at execution priority {}", hPostProcessPass.raw(), m_hPostProcessPasses.size() );
		}
		else
		{
			if ( m_hGraphicsPrePasses.size() <= executionPriority + 1 )
			{
				m_hPostProcessPasses.push_back( std::move( hPostProcessPass ) );
				MW_INFO( "Register Deffered Resolution Pre-Pass {} at execution priority {}", hPostProcessPass.raw(), m_hPostProcessPasses.size() );
			}
			else
			{
				m_hPostProcessPasses.insert( m_hPostProcessPasses.begin() + executionPriority, std::move( hPostProcessPass ) );
				MW_INFO( "Register Deffered Resolution Pre-Pass {} by inserting it at execution priority {}", hPostProcessPass.raw(), executionPriority );
			}
		}
	};

}
