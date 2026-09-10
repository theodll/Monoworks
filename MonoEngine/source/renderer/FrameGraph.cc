#include <mwpch.hh>

#include <utility>
#include <functional>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/PipelineManager.hh>

#include "StaticRenderer.hh"
#include "FrameGraph.hh"

namespace Monoworks
{
	using namespace RHI;
	constexpr u32 GlobalScopeSignature = 0;

	NODISCARD static Vector ComputeWorkgroupSize( slang::EntryPointReflection* pEntryPoint ) 
	{
		MW_PROFILE_FUNC;
		SExtent2D re = CStaticRenderer::GetRenderableExtend();
		
		SlangUInt x;
		SlangUInt y;
		SlangUInt z;

		pEntryPoint->getComputeThreadGroupSize( 1, &x );
		pEntryPoint->getComputeThreadGroupSize( 2, &y );
		pEntryPoint->getComputeThreadGroupSize( 3, &z );

		Vector out;
		out.x = re.Width / x;
		out.y = re.Height / y;
		out.z = z;

		return out;
	}

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

		// TODO: expand GraphicsPrePassCreationInfo because it needs a ton of more things
		// to even create a graphics pipeline.

		m_hShader = pInfo->hShader;
		m_pExecutionScopeCallback = pInfo->pExecutionScopeCallback;

		m_ColorAttachmentCount = pInfo->ColorAttachmentCount;
	
		m_ColorAttachments.clear();

		for ( auto i{ 0uz }; i < pInfo->ColorAttachmentCount; i++ )
			m_ColorAttachments.push_back( pInfo->pColorAttachments[i] );
	
		if ( pInfo->pDepthAttachment )
		{
			m_DepthAttachment = *pInfo->pDepthAttachment;
			m_Flags |= MW_GRAPHICS_PRE_PASS_USE_DEPTH_ATTACHMENT;
		}
		else
			m_DepthAttachment = {};

		if ( pInfo->pStencilAttachment )
		{
			m_StencilAttachment = *pInfo->pDepthAttachment;
			m_Flags |= MW_GRAPHICS_PRE_PASS_USE_STENCIL_ATTACHMENT;

		}
		else
			m_DepthAttachment = {};


		/*
		  NOTE: From GPassBase.slang
				public struct VertexInput
				{
					public float3       Position         : POSITION;
					public float3       Normal           : NORMAL;
					public float3       Tangent          : TANGENT;
					public float3       Binormal         : BINORMAL;
					public float2       TexCoord         : TEXCOORD0;
				}
		*/

		CVertexLayout defaultVertexLayout =
		{
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Position" },
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Normal"   },
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Tangent"  },
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Binormal" },
			{ MW_SHADER_DATA_TYPE_FLOAT_2, "TexCoord" }
		};

		auto entrypoints = m_hShader->GetShaderEntrypoints();
		const char* vertexEntrypoint = entrypoints[MW_SHADER_STAGE_VERTEX].c_str();

		SShaderObject vertexShader;
		vertexShader.ShaderStage = MW_SHADER_STAGE_VERTEX;
		vertexShader.pEntrypoint = vertexEntrypoint;

		auto program = m_hShader->GetShaderProgram();
		slang::ProgramLayout* layout = program->getLayout();

		{

			SlangInt vertexEntryPointIndex = -1;
			SlangInt entryPointCount = layout->getEntryPointCount();

			for ( SlangInt i = 0; i < entryPointCount; ++i )
			{
				slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );

				if ( entryPointLayout->getStage() == SLANG_STAGE_VERTEX )
				{
					vertexEntryPointIndex = i;
					break;
				}
			}

			if ( vertexEntryPointIndex != -1 )
			{
				Slang::ComPtr<slang::IBlob> code;
				Slang::ComPtr<slang::IBlob> diagnostics;
				const SlangInt targetIndex = 0;
				SlangResult result = program->getEntryPointCode(
					vertexEntryPointIndex,
					targetIndex,
					code.writeRef(),
					diagnostics.writeRef()
				);

				if ( SLANG_FAILED( result ) )
				{
					if ( diagnostics )
						MW_ERROR( "Failed to get vertex shader entry point code for graphics pre-pass pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

					return;
				}

				vertexShader.Code = { code->getBufferPointer(), code->getBufferSize() };

			}
		}

		const char* fragmentEntrypoint = entrypoints[MW_SHADER_STAGE_VERTEX].c_str();

		SShaderObject pixelShader;
		pixelShader.ShaderStage = MW_SHADER_STAGE_FRAGMENT;
		pixelShader.pEntrypoint = fragmentEntrypoint;

		{

			SlangInt vertexEntryPointIndex = -1;
			SlangInt entryPointCount = layout->getEntryPointCount();

			for ( SlangInt i = 0; i < entryPointCount; ++i )
			{
				slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );

				if ( entryPointLayout->getStage() == SLANG_STAGE_FRAGMENT )
				{
					vertexEntryPointIndex = i;
					break;
				}
			}

			if ( vertexEntryPointIndex != -1 )
			{
				Slang::ComPtr<slang::IBlob> code;
				Slang::ComPtr<slang::IBlob> diagnostics;
				const SlangInt targetIndex = 0;
				SlangResult result = program->getEntryPointCode(
					vertexEntryPointIndex,
					targetIndex,
					code.writeRef(),
					diagnostics.writeRef()
				);

				if ( SLANG_FAILED( result ) )
				{
					if ( diagnostics )
						MW_ERROR( "Failed to get pixel shader entry point code for graphics pre-pass pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

					return;
				}

				vertexShader.Code = { code->getBufferPointer(), code->getBufferSize() };

			}
		}

		std::vector<SShaderObject> shaderObjects;
		shaderObjects.push_back( vertexShader );
		shaderObjects.push_back( pixelShader );

		std::vector<SColorBlendAttachmentState> colorBlendAttachments;
		colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );


		auto pipelineReflectData = m_hShader->ReflectOnShader();

		RHI::GraphicsPipelineCreationInfo createInfo{};
		createInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
		createInfo.VertexLayout = defaultVertexLayout;
		createInfo.ShaderObjects = shaderObjects;
		createInfo.pSignature = pipelineReflectData.pPipelineSignature;
		createInfo.DepthAttachmentFormat = pInfo->DepthFormat;
		createInfo.StencilAttachmentFormat = pInfo->StencilFormat;

		auto basePassPipeline = RHI::CPipelineManager::CreateGraphicsPipeline( &createInfo, &m_PipelineHash );

		if ( basePassPipeline )
			m_hGraphicsPipeline = basePassPipeline.value();
		else
			MW_ERROR( "Failed to create graphics pre-pass pipeline." );
		// TODO: Implement error handling here.
	
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
			m_pDescriptors[i] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[GlobalScopeSignature] );


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
					frameDescriptors[i] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
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
		{
			ShaderCreateInfo shaderInfo{};
			shaderInfo.Flags = MW_SHADER_FLAG_INTERNAL_SLANG_SESSION;
			shaderInfo.Path = "shaders/DefaultMaterialStatic.slang"; // TODO: not hardcode this

			m_hDefaultBasePassShader = Ref<CShader>::Create( &shaderInfo );

			/*
			  NOTE: From GPassBase.slang
					public struct VertexInput
					{
						public float3       Position         : POSITION;
						public float3       Normal           : NORMAL;
						public float3       Tangent          : TANGENT;
						public float3       Binormal         : BINORMAL;
						public float2       TexCoord         : TEXCOORD0;
					}
			*/

			CVertexLayout basePassVertexLayout =
			{
				{ MW_SHADER_DATA_TYPE_FLOAT_3, "Position" },
				{ MW_SHADER_DATA_TYPE_FLOAT_3, "Normal"   },
				{ MW_SHADER_DATA_TYPE_FLOAT_3, "Tangent"  },
				{ MW_SHADER_DATA_TYPE_FLOAT_3, "Binormal" },
				{ MW_SHADER_DATA_TYPE_FLOAT_2, "TexCoord" }
			};

			auto entrypoints = m_hDefaultBasePassShader->GetShaderEntrypoints();
			const char* vertexEntrypoint = entrypoints[MW_SHADER_STAGE_VERTEX].c_str();

			SShaderObject vertexShader;
			vertexShader.ShaderStage = MW_SHADER_STAGE_VERTEX;
			vertexShader.pEntrypoint = vertexEntrypoint;

			auto program = m_hDefaultBasePassShader->GetShaderProgram();
			slang::ProgramLayout* layout = program->getLayout();

			{

				SlangInt vertexEntryPointIndex = -1;
				SlangInt entryPointCount = layout->getEntryPointCount();

				for ( SlangInt i = 0; i < entryPointCount; ++i )
				{
					slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );

					if ( entryPointLayout->getStage() == SLANG_STAGE_VERTEX )
					{
						vertexEntryPointIndex = i;
						break;
					}
				}

				if ( vertexEntryPointIndex != -1 )
				{
					Slang::ComPtr<slang::IBlob> code;
					Slang::ComPtr<slang::IBlob> diagnostics;
					const SlangInt targetIndex = 0;
					SlangResult result = program->getEntryPointCode(
						vertexEntryPointIndex,
						targetIndex,
						code.writeRef(),
						diagnostics.writeRef()
					);

					if ( SLANG_FAILED( result ) )
					{
						if ( diagnostics )
							MW_ERROR( "Failed to get vertex shader entry point code for default base pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

						return;
					}

					vertexShader.Code = { code->getBufferPointer(), code->getBufferSize() };

				}
			}

			const char* fragmentEntrypoint = entrypoints[MW_SHADER_STAGE_VERTEX].c_str();

			SShaderObject pixelShader;
			pixelShader.ShaderStage = MW_SHADER_STAGE_FRAGMENT;
			pixelShader.pEntrypoint = fragmentEntrypoint;

			{

				SlangInt vertexEntryPointIndex = -1;
				SlangInt entryPointCount = layout->getEntryPointCount();

				for ( SlangInt i = 0; i < entryPointCount; ++i )
				{
					slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );

					if ( entryPointLayout->getStage() == SLANG_STAGE_FRAGMENT )
					{
						vertexEntryPointIndex = i;
						break;
					}
				}

				if ( vertexEntryPointIndex != -1 )
				{
					Slang::ComPtr<slang::IBlob> code;
					Slang::ComPtr<slang::IBlob> diagnostics;
					const SlangInt targetIndex = 0;
					SlangResult result = program->getEntryPointCode(
						vertexEntryPointIndex,
						targetIndex,
						code.writeRef(),
						diagnostics.writeRef()
					);

					if ( SLANG_FAILED( result ) )
					{
						if ( diagnostics )
							MW_ERROR( "Failed to get pixel shader entry point code for default base pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

						return;
					}

					vertexShader.Code = { code->getBufferPointer(), code->getBufferSize() };

				}
			}

			std::vector<SShaderObject> shaderObjects;
			shaderObjects.push_back( vertexShader );
			shaderObjects.push_back( pixelShader );

			std::vector<EImageFormat> colorFormats;
			colorFormats.emplace_back( MW_FORMAT_R8G8B8A8_UNORM ); // Albedo + Occlusion 
			colorFormats.emplace_back( MW_FORMAT_A2R10G10B10_UNORM_PACK32 ); // Normals, Roughness + Metallicness 
			colorFormats.emplace_back( MW_FORMAT_R8G8B8_UNORM ); // Emissive
			colorFormats.emplace_back( MW_FORMAT_R16G16_SFLOAT ); // Motion vectors
			colorFormats.emplace_back( MW_FORMAT_R32_UINT ); // Entity ID (bits 0-18) + Material ID (bits 19-31)

			std::vector<SColorBlendAttachmentState> colorBlendAttachments;
			colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );


			auto pipelineReflectData = m_hDefaultBasePassShader->ReflectOnShader();

			RHI::GraphicsPipelineCreationInfo createInfo{};
			createInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT;
			createInfo.VertexLayout = basePassVertexLayout;
			createInfo.ShaderObjects = shaderObjects;
			createInfo.ColorFormats = colorFormats;
			createInfo.ColorBlendAttachments = colorBlendAttachments;
			createInfo.pSignature = pipelineReflectData.pPipelineSignature;
			createInfo.DepthAttachmentFormat = MW_FORMAT_D32_SFLOAT;

			auto basePassPipeline = RHI::CPipelineManager::CreateGraphicsPipeline( &createInfo, &m_DefaultBasePassPipelineHash );

			if ( basePassPipeline )
				m_hDefaultBasePassPipeline = basePassPipeline.value();
			else
				MW_FATAL( "Failed to create base pass pipeline." );
			// TODO: Implement error handling here.
		}




	};

	CDefferedFrameGraph::~CDefferedFrameGraph()	 NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_hComputePrePasses.clear();
		m_hGraphicsPrePasses.clear();
		m_hDefferedResolutionPasses.clear();
		m_hPostProcessPasses.clear();
	};

	void CDefferedFrameGraph::AddPrePass( Ref<CComputePrePass> hComputePrePass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( !hComputePrePass )
		{
			MW_API_WARN( "Passed invalid hComputePrePass to CDefferedFrameGraph::AddPrePass (Compute). Discarding.");
			return;
		}

		 
		if ( m_hComputePrePasses.size() <= executionPriority + 1 || executionPriority < 0 )
		{
			m_hComputePrePasses.push_back( std::move( hComputePrePass ) );
			MW_INFO( "Register Compute Pre-Pass {} at execution priority {}", hComputePrePass.raw(), m_hComputePrePasses.size() );
		}
		else
		{
			m_hComputePrePasses.insert( m_hComputePrePasses.begin() + executionPriority, std::move( hComputePrePass ) );
			MW_INFO( "Register Compute Pre-Pass {} by inserting it at execution priority {}", hComputePrePass.raw(), executionPriority );
		}
	};

	void CDefferedFrameGraph::AddPrePass( Ref<CGraphicsPrePass> hGraphicsPrePass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( !hGraphicsPrePass )
		{
			MW_API_WARN( "Passed invalid hGraphicsPrePass to CDefferedFrameGraph::AddPrePass (Graphics). Discarding." );
			return;
		}

		if ( m_hGraphicsPrePasses.size() <= executionPriority + 1 || executionPriority < 0 )
		{
			m_hGraphicsPrePasses.push_back( std::move( hGraphicsPrePass ) );
			MW_INFO( "Register Graphics Pre-Pass {} at execution priority {}", ( void* )hGraphicsPrePass.raw(), m_hGraphicsPrePasses.size() );
		}
		else
		{
			m_hGraphicsPrePasses.insert( m_hGraphicsPrePasses.begin() + executionPriority, std::move( hGraphicsPrePass ) );
			MW_INFO( "Register Graphics Pre-Pass {} by inserting it at execution priority {}", ( void* )hGraphicsPrePass.raw(), executionPriority );
		}
	};

	void CDefferedFrameGraph::AddDefferedResolutionPass( Ref<CDefferedResolutionPass> hComputePass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hComputePass )
		{
			MW_API_WARN( "Passed invalid hComputePass to CDefferedFrameGraph::AddDefferedResolutionPass. Discarding." );
			return;
		}

		if ( m_hGraphicsPrePasses.size() <= executionPriority + 1 || executionPriority < 0 )
		{
			m_hDefferedResolutionPasses.push_back( std::move( hComputePass ) );
			MW_INFO( "Register Deffered Resolution Pass {} at execution priority {}", ( void* )hComputePass.raw(), m_hDefferedResolutionPasses.size() );
		}
		else
		{
			m_hDefferedResolutionPasses.insert( m_hDefferedResolutionPasses.begin() + executionPriority, std::move( hComputePass ) );
			MW_INFO( "Register Deffered Resolution Pass {} by inserting it at execution priority {}", ( void* )hComputePass.raw(), executionPriority );
		}
	};

	void CDefferedFrameGraph::AddPostProcessPass( Ref<CPostProcessPass> hPostProcessPass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hPostProcessPass )
		{
			MW_API_WARN( "Passed invalid hPostProcessPass to CDefferedFrameGraph::AddPostProcessPass. Discarding." );
			return;
		}

		// Check if either the execution priority would be bi
		if ( m_hGraphicsPrePasses.size() <= executionPriority + 1 || executionPriority < 0 )
		{
			// Insert the post process pass last.
			m_hPostProcessPasses.push_back( std::move( hPostProcessPass ) );
			MW_INFO( "Register Post Processing Pass {} at execution priority {}", ( void* )hPostProcessPass.raw(), m_hPostProcessPasses.size() );
		}
		else
		{
			// Insert the post process pass at the execution priority and shifting all other passes back one unit.
			m_hPostProcessPasses.insert( m_hPostProcessPasses.begin() + executionPriority, std::move( hPostProcessPass ) );
			MW_INFO( "Register Post Processing Pass {} by inserting it at execution priority {}", ( void* )hPostProcessPass.raw(), executionPriority );
		}
	};


	static void MW_NOTHROW BindCommandBufferStateBothBegun( u32 frameIndex ) 
	{
		MW_PROFILE_FUNC;

		CStaticRenderer::SetDynamicCullMode( frameIndex, MW_CULL_MODE_BACK );
		CStaticRenderer::SetDynamicScissors( frameIndex, &CStaticRenderer::GetRenderableExtend(), 1, 0 );

		Viewport viewport{};
		viewport.Width =  ( float )CStaticRenderer::GetRenderableExtend().Width;
		viewport.Height = ( float )CStaticRenderer::GetRenderableExtend().Height;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f; 

		CStaticRenderer::SetDynamicViewports( frameIndex, &viewport, 1, 0 );

	}

	// Binds static state only to the root command buffer. Use this if the worker commandbuffers have not started yet.
	static void MW_NOTHROW BindCommandBufferStateOnlyRoot( u32 frameIndex )
	{
		MW_PROFILE_FUNC;

		CStaticRenderer::SetDynamicCullModeST( frameIndex, MW_CULL_MODE_BACK, -1 );
		CStaticRenderer::SetDynamicScissorsST( frameIndex, &CStaticRenderer::GetRenderableExtend(), 1, 0, -1 );

		Viewport viewport{};
		viewport.Width = ( float )CStaticRenderer::GetRenderableExtend().Width;
		viewport.Height = ( float )CStaticRenderer::GetRenderableExtend().Height;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;

		CStaticRenderer::SetDynamicViewportsST( frameIndex, &viewport, 1, 0, -1 );

	}

	MW_NOTHROW void CDefferedFrameGraph::ExecutePreRenderingSetup() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		u32 frameIndex = CStaticRenderer::GetCurrentFrameIndex();

		CStaticRenderer::BeginRootCommandbuffer( frameIndex );
		BindCommandBufferStateOnlyRoot( frameIndex );
	}

	void CDefferedFrameGraph::ExecutePrePasses() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		u32 frameIndex = CStaticRenderer::GetCurrentFrameIndex();

		CStaticRenderer::BeginSecondaryCommandbuffers( frameIndex );
		BindCommandBufferStateBothBegun( frameIndex );

		// TODO: Convert this to a Job
		// TODO: cvars
		for ( auto& computePrePass : m_hComputePrePasses )
		{
			auto workgroup = ComputeWorkgroupSize( computePrePass->m_hShader->GetShaderProgram()->getLayout()->getEntryPointByIndex( 0 ) );
			CStaticRenderer::DispatchCompute( frameIndex, computePrePass->m_hComputePipeline, workgroup, -1, &computePrePass->m_pDescriptors[frameIndex], 1 );
		}

		// Merge secondaries because every graphics pre-pass must have their own secondaries. That's just the way it is I'm just the messenger
		CStaticRenderer::MergeSecondaryCommandbuffers( frameIndex ); 

		// TODO: Job
		for ( auto& graphicsPrePass : m_hGraphicsPrePasses )
		{
			RHI::BeginRenderingInfo info;
			info.Flags = MW_RENDERING_FLAGS_WITH_SECONDARY_COMMAND_BUFFERS_BIT;
			info.ColorAttachmentCount = graphicsPrePass->m_ColorAttachments.size();
			info.pColorAttachments = graphicsPrePass->m_ColorAttachments.data();
			info.pDepthAttachment = &graphicsPrePass->m_DepthAttachment;
			info.pStencilAttachment = &graphicsPrePass->m_StencilAttachment;
			info.RenderArea = graphicsPrePass->m_RenderingArea;
			// Begin the 
			CStaticRenderer::BeginSecondaryCommandbuffers( frameIndex );
			BindCommandBufferStateBothBegun( frameIndex );

			CStaticRenderer::BeginRendering( frameIndex, &info ); 

			// Bind the pre pass pipeline generated from graphicsPrePass::m_hShader and the descriptor for the shader globals.
			CStaticRenderer::BindGraphicsPipeline( frameIndex, graphicsPrePass->m_hGraphicsPipeline );
			CStaticRenderer::BindDescriptors( frameIndex, *graphicsPrePass->m_hGraphicsPipeline->GetSignature(), &graphicsPrePass->m_pDescriptors[frameIndex], 1, 0, -1);

			// Invoke the execution callback specified by the pre-pass. The pre-pass manager can submit draw calls, bind state etc. here.
			if ( graphicsPrePass->m_pExecutionScopeCallback )
				std::invoke( graphicsPrePass->m_pExecutionScopeCallback, frameIndex );

			// Merge before ending rendering, as specified by the Vulkan Specification. 
			CStaticRenderer::MergeSecondaryCommandbuffers( frameIndex );

			CStaticRenderer::EndRendering( frameIndex );
		}
	};

	void CDefferedFrameGraph::ExecuteBasePasses() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		u32 frameIndex = CStaticRenderer::GetCurrentFrameIndex();
		// Base Pass


		// Deffered Resolution Pass
		for ( auto& resolutionPass : m_hDefferedResolutionPasses )
		{
			auto workgroup = ComputeWorkgroupSize( resolutionPass->m_hShader->GetShaderProgram()->getLayout()->getEntryPointByIndex( 0 ) );

			// TODO: Do this batching automatically by reordering the descriptors array.
			// Batch descriptors.
			std::vector<DescriptorHandle> descriptors;

			// Globals/set 0 must be first in the descriptors array.
			descriptors.push_back( resolutionPass->m_pDescriptors[0][frameIndex] );

			// Only run this loop if necessary.
			if ( resolutionPass->m_pDescriptors.size() > 1 )
				for ( auto i{ 1uz }; i < resolutionPass->m_pDescriptors.size(); i++ )
				{
					descriptors.push_back( resolutionPass->m_pDescriptors[i][frameIndex] );
				}

			CStaticRenderer::DispatchCompute( frameIndex, resolutionPass->m_hComputePipeline, workgroup, -1, descriptors.data(), descriptors.size() );
		}
		
	};

	void CDefferedFrameGraph::ExecutePostPasses() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		u32 frameIndex = CStaticRenderer::GetCurrentFrameIndex();
		
		for ( auto& postPass : m_hPostProcessPasses )
		{
			// TODO: Jobs
			auto workgroup = ComputeWorkgroupSize( postPass->m_hShader->GetShaderProgram()->getLayout()->getEntryPointByIndex( 0 ) );
			CStaticRenderer::DispatchCompute( frameIndex, postPass->m_hComputePipeline, workgroup, -1, &postPass->m_pDescriptors[CStaticRenderer::GetCurrentFrameIndex()], 1 );
		}
	};


}
