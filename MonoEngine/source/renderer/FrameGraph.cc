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

	CPostProcessPass::CPostProcessPass( PostProcessPassCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		
		if ( !pInfo->hShader )
		{
			MW_API_ERROR( "Invalid Shader Reference Passed" );
			throw std::runtime_error( "Invalid Shader Reference Passed" );
			return;
		};
		
		auto reflectionData = pInfo->hShader->ReflectOnShader();
		auto entrypoints = pInfo->hShader->GetShaderEntrypoints();
		const char* entrypoint = entrypoints[MW_SHADER_STAGE_COMPUTE].c_str();

		Slang::ComPtr<slang::IBlob> code;
		Slang::ComPtr<slang::IBlob> diagnostics;

		const SlangInt entrypointPointIndex = 0;
		const SlangInt targetIndex = 0;

		const SlangResult result = 
			pInfo->hShader->GetShaderProgram()->getEntryPointCode(
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


	CDefferedResolutionPass::CDefferedResolutionPass( DefferedResolutionPassCreateionInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		MW_PROFILE_FUNC;

		if ( !pInfo->hShader )
		{
			MW_API_ERROR( "Invalid Shader Reference Passed" );
			throw std::runtime_error( "Invalid Shader Reference Passed" );
			return;
		};

		auto reflectionData = pInfo->hShader->ReflectOnShader();
		auto entrypoints = pInfo->hShader->GetShaderEntrypoints();
		const char* entrypoint = entrypoints[MW_SHADER_STAGE_COMPUTE].c_str();

		Slang::ComPtr<slang::IBlob> code;
		Slang::ComPtr<slang::IBlob> diagnostics;

		const SlangInt entrypointPointIndex = 0;
		const SlangInt targetIndex = 0;

		const SlangResult result =
			pInfo->hShader->GetShaderProgram()->getEntryPointCode(
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

}
