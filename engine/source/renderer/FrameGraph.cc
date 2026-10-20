#include <mwpch.hh>

#include <utility>
#include <functional>

#include <core/Application.hh>
#include <events/EventManager.hh>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/PipelineManager.hh>

#include <renderer/FrameManager.hh>

#include "StaticRenderer.hh"
#include "FrameGraph.hh"

#include "../rhi/specific/vulkan/VulkanRenderManager.hh"
#include "../rhi/specific/vulkan/VulkanTexture.hh"


namespace Monoworks
{
	// TODO: Refactor
	using namespace RHI;
	constexpr u32 GlobalScopeSignature = 0;

	NODISCARD static Vector ComputeWorkgroupSize( slang::EntryPointReflection* pEntryPoint ) 
	{
		MW_PROFILE_FUNC;
		SExtent2D re = CStaticRenderer::GetRenderableExtend();
		
		SlangUInt vec[3];

		pEntryPoint->getComputeThreadGroupSize( 3, vec );

		Vector out;
		out.x = re.Width / vec[0];
		out.y = re.Height / vec[1];
		out.z = vec[2];

		return out;
	}

	NODISCARD static bool IsParameterBlock( slang::VariableLayoutReflection* pVar )
	{
		if ( pVar == nullptr || pVar->getTypeLayout() == nullptr )
			return false;

		return pVar->getTypeLayout()->getKind() == slang::TypeReflection::Kind::ParameterBlock;
	}

	NODISCARD static bool HasGlobalBinding( std::string_view name, slang::ProgramLayout* pLayout )
	{
		MW_PROFILE_FUNC;

		for ( u32 i = 0; i < pLayout->getParameterCount(); ++i )
		{
			slang::VariableLayoutReflection* pParam = pLayout->getParameterByIndex( i );

			if ( pParam == nullptr )
				continue;

			if ( IsParameterBlock( pParam ) )
				continue;

			const char* pParamName = pParam->getName();
			if ( pParamName && name == pParamName )
				return true;
		}

		return false;
	}

	NODISCARD static u32 GetParameterBlockSet( slang::VariableLayoutReflection* pBlockVar )
	{
		return static_cast< u32 >( pBlockVar->getOffset( slang::ParameterCategory::SubElementRegisterSpace ) );
	}

	NODISCARD static slang::VariableLayoutReflection* FindParameterByName( std::string_view name, slang::ProgramLayout* pLayout )
	{
		for ( u32 i = 0; i < pLayout->getParameterCount(); ++i )
		{
			slang::VariableLayoutReflection* pParam = pLayout->getParameterByIndex( i );
			const char* pParamName = pParam ? pParam->getName() : nullptr;

			if ( pParamName && name == pParamName )
				return pParam;
		}

		return nullptr;
	}

	NODISCARD static bool HasParameterBlock( std::string_view parameterBlockName, slang::ProgramLayout* pLayout )
	{
		MW_PROFILE_FUNC;

		slang::VariableLayoutReflection* pBlockVar = FindParameterByName( parameterBlockName, pLayout );

		return IsParameterBlock( pBlockVar );
	}

	NODISCARD static std::expected<u32, EResult> FindFieldBindingInParameterBlock( slang::VariableLayoutReflection* pBlockVar, std::string_view fieldName )
	{
		slang::TypeLayoutReflection* pBlockTypeLayout = pBlockVar->getTypeLayout();
		slang::VariableLayoutReflection* pElementVar = pBlockTypeLayout->getElementVarLayout();

		if ( pElementVar == nullptr )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		slang::TypeLayoutReflection* pElementTypeLayout = pElementVar->getTypeLayout();
		const u32 elementBindingOffset = static_cast< u32 >( pElementVar->getOffset( slang::ParameterCategory::DescriptorTableSlot ) );

		for ( u32 i = 0; i < pElementTypeLayout->getFieldCount(); ++i )
		{
			slang::VariableLayoutReflection* pField = pElementTypeLayout->getFieldByIndex( i );
			const char* pFieldName = pField->getName();

			if ( pFieldName && fieldName == pFieldName )
				return elementBindingOffset + static_cast< u32 >( pField->getOffset( slang::ParameterCategory::DescriptorTableSlot ) );
		}

		return std::unexpected( MW_ERROR_NON_EXISTANT );
	}

	NODISCARD static std::expected<u32, EResult> FindParameterBlockNumberByString( std::string_view parameterBlockName, slang::ProgramLayout* pLayout )
	{
		MW_PROFILE_FUNC;

		slang::VariableLayoutReflection* pBlockVar = FindParameterByName( parameterBlockName, pLayout );

		if ( !IsParameterBlock( pBlockVar ) )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		return GetParameterBlockSet( pBlockVar );
	}

	NODISCARD static std::expected<std::pair<u32, u32>, EResult> FindParameterBlockAndBindingNumberByString( std::string_view parameterBlockName, std::string_view bindingName, slang::ProgramLayout* pLayout )
	{
		MW_PROFILE_FUNC;

		slang::VariableLayoutReflection* pBlockVar = FindParameterByName( parameterBlockName, pLayout );

		if ( !IsParameterBlock( pBlockVar ) )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		auto bindingIndex = FindFieldBindingInParameterBlock( pBlockVar, bindingName );

		if ( !bindingIndex )
			return std::unexpected( bindingIndex.error() );

		return std::make_pair( GetParameterBlockSet( pBlockVar ), bindingIndex.value() );
	}

	NODISCARD static std::expected<u32, EResult> FindBindingNumberByString( u32 parameterBlock, std::string_view name, slang::ProgramLayout* pLayout ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		for ( u32 i = 0; i < pLayout->getParameterCount(); ++i )
		{
			slang::VariableLayoutReflection* pParam = pLayout->getParameterByIndex( i );

			if ( pParam == nullptr )
				continue;

			const char* pParamName = pParam->getName();

			if ( IsParameterBlock( pParam ) )
			{
				if ( GetParameterBlockSet( pParam ) != parameterBlock )
					continue;

				if ( pParamName && name == pParamName )
				{
					slang::VariableLayoutReflection* pContainerVar = pParam->getTypeLayout()->getContainerVarLayout();

					if ( pContainerVar == nullptr )
						return 0u;

					return static_cast< u32 >( pContainerVar->getOffset( slang::ParameterCategory::DescriptorTableSlot ) );
				}

				auto fieldBinding = FindFieldBindingInParameterBlock( pParam, name );

				if ( fieldBinding )
					return fieldBinding.value();
			}
			else
			{
				if ( pParamName == nullptr || name != pParamName )
					continue;

				if ( pParam->getBindingSpace( slang::ParameterCategory::DescriptorTableSlot ) != parameterBlock )
					continue;

				return static_cast< u32 >( pParam->getOffset( slang::ParameterCategory::DescriptorTableSlot ) );
			}
		}

		return std::unexpected( MW_ERROR_NON_EXISTANT );
	}
	
	CComputeRenderPass::CComputeRenderPass( const ComputeRenderPassCreationInfo* pInfo ) NOEXCEPT : IRenderPass( pInfo->hShader )
	{
		MW_PROFILE_FUNC;

		MW_INFO( "Instatiate CComputeRenderPass");

		if ( !pInfo->hShader )
		{
			MW_API_ERROR( "Invalid Shader Reference Passed" );
			return;
		};

		m_hShader = pInfo->hShader;

		const auto reflectionData = m_hShader->ReflectOnShader();

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
		computeShader.pEntrypoint = "main";
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

		if ( m_hShader->HasGlobalBindings() )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				m_pDescriptors[i].resize( GlobalScopeSignature + 1 );
				m_pDescriptors[i][GlobalScopeSignature] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[GlobalScopeSignature] );
			}
		}
		else
			MW_WARN( "Reflection did not yield global bindings. Discarded allocation of global scope descriptors." );
	}

	CComputeRenderPass::~CComputeRenderPass() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CPipelineManager::DeleteComputePipeline( m_PipelineHash );
		m_hComputePipeline = nullptr;
	}

	CGraphicsRenderPass::CGraphicsRenderPass( const GraphicsRenderPassCreationInfo* pInfo ) NOEXCEPT : IRenderPass( pInfo->hShader )
	{
		MW_PROFILE_FUNC;
		MW_INFO( "Instatiate CGraphicsRenderPass" );
		// TODO: expand GraphicsPrePassCreationInfo because it needs a ton of more things
		// to even create a graphics pipeline.

		m_hShader = pInfo->hShader;
		if ( pInfo->pExecutionScopeCallback )
			m_pExecutionScopeCallback = pInfo->pExecutionScopeCallback.value();

		m_RenderingArea = pInfo->RenderingArea;
		m_PipelineHash = 0;
		
		auto pipelineReflectData = m_hShader->ReflectOnShader();

		if ( pInfo->ppColorAttachments )
		{
			std::array<std::vector<Monoworks::RHI::RenderingAttachmentInfo>, MFIF> tempAttachments;

			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				auto& span = pInfo->ppColorAttachments.value()[i];
				tempAttachments[i].assign( span.begin(), span.end() );
			}

			m_ColorAttachments = std::move( tempAttachments );
		}
		else
		{
			m_ColorAttachments = std::nullopt;
		}


		if ( pInfo->pDepthAttachment )
		{
			std::array<Monoworks::RHI::RenderingAttachmentInfo, MFIF> temp{};

			std::ranges::copy(
				*pInfo->pDepthAttachment,
				temp.begin()
			);

			m_DepthAttachment = std::move( temp );
		}

		if ( pInfo->pStencilAttachment )
		{
			std::array<Monoworks::RHI::RenderingAttachmentInfo, MFIF> temp{};

			std::ranges::copy(
				*pInfo->pStencilAttachment,
				temp.begin()
			);

			m_StencilAttachment = std::move( temp );
		}

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

		std::vector<SShaderObject> shaderObjects;

		CVertexLayout defaultVertexLayout =
		{
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Position" },
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Normal"   },
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Tangent"  },
			{ MW_SHADER_DATA_TYPE_FLOAT_3, "Binormal" },
			{ MW_SHADER_DATA_TYPE_FLOAT_2, "TexCoord" }
		};

		auto entrypoints = m_hShader->GetShaderEntrypoints();
		const char* vertexEntrypoint = entrypoints[MW_SHADER_STAGE_VERTEX].data();

		SShaderObject vertexShader;
		vertexShader.ShaderStage = MW_SHADER_STAGE_VERTEX;
		vertexShader.pEntrypoint = "main"; // TODO: Dont do this

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

			shaderObjects.push_back( vertexShader );
		}

		if ( !( pInfo->Flags & MW_GRAPHICS_PRE_PASS_CREATION_FLAGS_DISABLE_PIXEL_SHADER_BIT ) )
		{
			const char* fragmentEntrypoint = entrypoints[MW_SHADER_STAGE_FRAGMENT].data();

			SShaderObject pixelShader;
			pixelShader.ShaderStage = MW_SHADER_STAGE_FRAGMENT;
			pixelShader.pEntrypoint = fragmentEntrypoint;

			{

				SlangInt pixelEntryPointIndex = -1;
				SlangInt entryPointCount = layout->getEntryPointCount();

				for ( SlangInt i = 0; i < entryPointCount; ++i )
				{
					slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );

					if ( entryPointLayout->getStage() == SLANG_STAGE_FRAGMENT )
					{
						pixelEntryPointIndex = i;
						break;
					}
				}

				if ( pixelEntryPointIndex != -1 )
				{
					Slang::ComPtr<slang::IBlob> code;
					Slang::ComPtr<slang::IBlob> diagnostics;
					const SlangInt targetIndex = 0;
					SlangResult result = program->getEntryPointCode(
						pixelEntryPointIndex,
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

					pixelShader.Code = { code->getBufferPointer(), code->getBufferSize() };

				}
			}
			shaderObjects.push_back( pixelShader );
		}
		
		std::vector<SColorBlendAttachmentState> colorBlendAttachments;
		if ( pInfo->ppColorAttachments )
		{
			for ( auto i{ 0uz }; i < pInfo->ppColorAttachments.value()[0].size(); i++ )
				colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );
		}
		

		RHI::GraphicsPipelineCreationInfo createInfo{};
		createInfo.Flags = 0;
		createInfo.VertexLayout = defaultVertexLayout;
		createInfo.ShaderObjects = shaderObjects;
		createInfo.pSignature = pipelineReflectData.pPipelineSignature;
		createInfo.ColorFormats.clear();
		createInfo.ColorBlendAttachments = colorBlendAttachments;
		createInfo.DepthAttachmentFormat = pInfo->DepthFormat;
		createInfo.StencilAttachmentFormat = pInfo->StencilFormat;

		auto basePassPipeline = RHI::CPipelineManager::CreateGraphicsPipeline( &createInfo, &m_PipelineHash, false );

		if ( basePassPipeline )
			m_hGraphicsPipeline = basePassPipeline.value();
		else
			MW_ERROR( "Failed to create graphics pre-pass pipeline." );

		if ( m_hShader->HasGlobalBindings() )
		{
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				m_pDescriptors[i].resize( GlobalScopeSignature + 1 );
				m_pDescriptors[i][GlobalScopeSignature] = CDescriptorManager::Allocate( pipelineReflectData.pDescriptorSignatures[GlobalScopeSignature] );
			}

			m_FirstSet = GlobalScopeSignature;
		}
		else
			MW_WARN( "Reflection did not yield global bindings. Discarded allocation of global scope descriptors." );
		
		m_FirstSet = 0;
		
		// TODO: Implement error handling here.
	
	}

	 CGraphicsRenderPass::~CGraphicsRenderPass() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		CPipelineManager::DeleteGraphicsPipeline( m_PipelineHash );
		m_hGraphicsPipeline = nullptr;
	}

	 IRenderPass::IRenderPass(Ref<CShader> hShader) NOEXCEPT : m_hShader(hShader)
	 {
		 MW_PROFILE_FUNC;
	 }


	 void MWAPI_CALL IRenderPass::BindTexture( 
		std::string_view parameterBlockName, 
		std::string_view bindingName,
		Ref<RHI::ITexture2D> hTexture,
		bool forceRewrite /*= false */, 
		RHI::EDescriptorType imageType ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( !hTexture )
		{
			MW_API_ERROR( "Passed invalid texture reference." );
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

		// TODO: Don't base it on current frame index. iterate over array instead.

		bool rewrite = forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_pDescriptors[i].size() <= parameterBlock )
				m_pDescriptors[i].resize(parameterBlock + 1);

			if ( m_pDescriptors[i][parameterBlock] == nullptr )
			{
				auto reflectionData = m_hShader->ReflectOnShader();
				if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
				{
					MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
					return;
				}
			
				m_pDescriptors[i][parameterBlock] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
			}


			if ( rewrite )
			{
				CDescriptorManager::WriteImage( m_pDescriptors[i][parameterBlock], descriptorSlot, hTexture, imageType );

				m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
			}
		}

	}

	void MWAPI_CALL IRenderPass::BindTexture( 
		std::string_view bindingName, 
		Ref<RHI::ITexture2D> hTexture, 
		bool forceRewrite /*= false */,
		RHI::EDescriptorType imageType ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hTexture )
		{
			MW_API_ERROR( "Passed invalid texture reference." );
			return;
		}

		if ( !m_hShader->HasGlobalBindings() )
		{
			MW_API_ERROR( "Try to bind Texture to Global Scope without globals defined." );
			return;
		}

		auto binding = FindBindingNumberByString( GlobalScopeSignature, bindingName, m_hShader->GetShaderProgram()->getLayout() );
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
				CDescriptorManager::WriteImage( m_pDescriptors[i][GlobalScopeSignature], binding.value(), hTexture, imageType );

			m_BindingsWritten[{GlobalScopeSignature, binding.value()}] = true;

		}
	}

	void MWAPI_CALL IRenderPass::BindTexture( 
		std::string_view bindingName,
		std::span<Ref<RHI::ITexture2D>, MFIF> hTextures, 
		bool forceRewrite /*= false*/, 
		RHI::EDescriptorType imageType /*= RHI::MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( hTextures.empty() )
		{
			MW_API_ERROR( "Passed invalid texture reference." );
			return;
		}

		if ( !m_hShader->HasGlobalBindings() )
		{
			MW_API_ERROR( "Try to bind Texture to Global Scope without globals defined." );
			return;
		}

		auto binding = FindBindingNumberByString( GlobalScopeSignature, bindingName, m_hShader->GetShaderProgram()->getLayout() );
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

		auto descriptorSlot = binding.value();
		bool rewrite = forceRewrite || !m_BindingsWritten[{GlobalScopeSignature, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( !hTextures[i] )
			{
				MW_API_ERROR( "Passed invalid texture reference." );
				return;
			}

			if ( rewrite )
			{
				CDescriptorManager::WriteImage( m_pDescriptors[i][GlobalScopeSignature], descriptorSlot, hTextures[i], imageType );

				m_BindingsWritten[{GlobalScopeSignature, descriptorSlot}] = true;
			}
		}

	}

	void MWAPI_CALL IRenderPass::BindTexture(
		std::string_view parameterBlockName,
		std::string_view bindingName,
		std::span<Ref<RHI::ITexture2D>, MFIF> hTextures,
		bool forceRewrite /*= false */,
		RHI::EDescriptorType imageType /*= RHI::MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE */ ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( hTextures.empty() )
		{
			MW_API_ERROR( "Passed invalid texture reference." );
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
		bool rewrite = forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_pDescriptors[i].size() <= parameterBlock )
				m_pDescriptors[i].resize( parameterBlock + 1 );

			if ( !hTextures[i] )
			{
				MW_API_ERROR( "Passed invalid texture reference." );
				return;
			}

			if ( m_pDescriptors[i][parameterBlock] == nullptr )
			{
				auto reflectionData = m_hShader->ReflectOnShader();
				if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
				{
					MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
					return;
				}

				m_pDescriptors[i][parameterBlock] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
			}


			if ( rewrite )
			{
				CDescriptorManager::WriteImage( m_pDescriptors[i][parameterBlock], descriptorSlot, hTextures[i], imageType );

				m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
			}
		}
	};

	void MWAPI_CALL IRenderPass::BindUBO(
		std::string_view bindingName,
		Ref<RHI::IUniformBuffer> hUniformBuffer,
		bool forceRewrite ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hUniformBuffer )
		{
			MW_API_ERROR( "Passed invalid uniform buffer reference." );
			return;
		}

		if ( !m_hShader->HasGlobalBindings() )
		{
			MW_API_ERROR( "Try to bind Texture to Global Scope without globals defined." );
			return;
		}

		auto binding = FindBindingNumberByString( GlobalScopeSignature, bindingName, m_hShader->GetShaderProgram()->getLayout() );
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

		auto descriptorSlot = binding.value();
		bool rewrite = forceRewrite || !m_BindingsWritten[{GlobalScopeSignature, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{

			if ( rewrite )
			{
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[i][GlobalScopeSignature], descriptorSlot, hUniformBuffer );

				m_BindingsWritten[{GlobalScopeSignature, descriptorSlot}] = true;
			}
		}
	};

	void MWAPI_CALL IRenderPass::BindUBO(
		std::string_view parameterBlockName,
		std::string_view bindingName,
		Ref<RHI::IUniformBuffer> hUniformBuffer,
		bool forceRewrite ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( !hUniformBuffer )
		{
			MW_API_ERROR( "Passed invalid uniform buffer reference." );
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
		bool rewrite = forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_pDescriptors[i].size() <= parameterBlock )
				m_pDescriptors[i].resize( parameterBlock + 1 );

			if ( m_pDescriptors[i][parameterBlock] == nullptr )
			{
				auto reflectionData = m_hShader->ReflectOnShader();
				if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
				{
					MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
					return;
				}

				m_pDescriptors[i][parameterBlock] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
			}


			if ( rewrite )
			{
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[i][parameterBlock], descriptorSlot, hUniformBuffer );

				m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
			}
		}
	};

	void MWAPI_CALL IRenderPass::BindUBO(
		std::string_view bindingName,
		std::span<Ref<RHI::IUniformBuffer>, MFIF> hUniformBuffers,
		bool forceRewrite ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( hUniformBuffers.empty() )
		{
			MW_API_ERROR( "Passed invalid uniform buffer reference." );
			return;
		}

		if ( !m_hShader->HasGlobalBindings() )
		{
			MW_API_ERROR( "Try to bind Texture to Global Scope without globals defined." );
			return;
		}

		auto binding = FindBindingNumberByString( GlobalScopeSignature, bindingName, m_hShader->GetShaderProgram()->getLayout() );
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

		auto descriptorSlot = binding.value();
		bool rewrite = forceRewrite || !m_BindingsWritten[{GlobalScopeSignature, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( !hUniformBuffers[i] )
			{
				MW_API_ERROR( "Passed invalid texture reference." );
				return;
			}

			if ( rewrite )
			{
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[i][GlobalScopeSignature], descriptorSlot, hUniformBuffers[i] );

				m_BindingsWritten[{GlobalScopeSignature, descriptorSlot}] = true;
			}
		}
	};

	void MWAPI_CALL IRenderPass::BindUBO(
		std::string_view parameterBlockName,
		std::string_view bindingName,
		std::span<Ref<RHI::IUniformBuffer>, MFIF> hUniformBuffers,
		bool forceRewrite ) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( hUniformBuffers.empty() )
		{
			MW_API_ERROR( "Passed invalid uniform buffer reference." );
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
		bool rewrite = forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_pDescriptors[i].size() <= parameterBlock )
				m_pDescriptors[i].resize( parameterBlock + 1 );

			if ( !hUniformBuffers[i] )
			{
				MW_API_ERROR( "Passed invalid texture reference." );
				return;
			}

			if ( m_pDescriptors[i][parameterBlock] == nullptr )
			{
				auto reflectionData = m_hShader->ReflectOnShader();
				if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
				{
					MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
					return;
				}

				m_pDescriptors[i][parameterBlock] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
			}


			if ( rewrite )
			{
				CDescriptorManager::WriteUniformBuffer( m_pDescriptors[i][parameterBlock], descriptorSlot, hUniformBuffers[i] );

				m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
			}
		}
	};

	void MWAPI_CALL IRenderPass::BindSampler(
		std::string_view parameterBlockName,
		std::string_view bindingName,
		Ref<RHI::ITexture2D> hSampler,
		bool forceRewrite /*= false */ ) NOEXCEPT
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
		bool rewrite = forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_pDescriptors[i].size() <= parameterBlock )
				m_pDescriptors[i].resize( parameterBlock + 1 );

			if ( m_pDescriptors[i][parameterBlock] == nullptr )
			{
				auto reflectionData = m_hShader->ReflectOnShader();
				if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
				{
					MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
					return;
				}

				m_pDescriptors[i][parameterBlock] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
			}

			if ( rewrite )
			{
				CDescriptorManager::WriteSampler( m_pDescriptors[i][parameterBlock], descriptorSlot, hSampler );

				m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
			}
		}
	}

	void MWAPI_CALL IRenderPass::BindSampler( 
		std::string_view bindingName,
		Ref<RHI::ITexture2D> hSampler,
		bool forceRewrite /*= false */ ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hSampler )
		{
			MW_API_ERROR( "Passed invalid Sampler reference." );
			return;
		}

		if ( !m_hShader->HasGlobalBindings() )
		{
			MW_API_ERROR( "Try to bind Sampler to Global Scope without globals defined." );
			return;
		}

		auto binding = FindBindingNumberByString( GlobalScopeSignature, bindingName, m_hShader->GetShaderProgram()->getLayout() );
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
				CDescriptorManager::WriteSampler( m_pDescriptors[i][GlobalScopeSignature], binding.value(), hSampler );

			m_BindingsWritten[{GlobalScopeSignature, binding.value()}] = true;
		}
	}

	void MWAPI_CALL IRenderPass::BindSampler(
		std::string_view bindingName,
		std::span<Ref<RHI::ITexture2D>, MFIF> hSamplers,
		bool forceRewrite /* = false */) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( hSamplers.empty() )
		{
			MW_API_ERROR( "Passed invalid Sampler reference." );
			return;
		}

		if ( !m_hShader->HasGlobalBindings() )
		{
			MW_API_ERROR( "Try to bind Sampler to Global Scope without globals defined." );
			return;
		}

		auto binding = FindBindingNumberByString( GlobalScopeSignature, bindingName, m_hShader->GetShaderProgram()->getLayout() );
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
			{
				if ( !hSamplers[i] )
				{
					MW_API_ERROR( "Passed invalid Sampler reference." );
					return;
				}
				CDescriptorManager::WriteSampler( m_pDescriptors[i][GlobalScopeSignature], binding.value(), hSamplers[i] );
			}

			m_BindingsWritten[{GlobalScopeSignature, binding.value()}] = true;
		}
	};

	void MWAPI_CALL IRenderPass::BindSampler(
		std::string_view parameterBlockName,
		std::string_view bindingName,
		std::span<Ref<RHI::ITexture2D>, MFIF> hSamplers,
		bool forceRewrite /* = false  */) NOEXCEPT 
	{
		MW_PROFILE_FUNC;

		auto bindings = FindParameterBlockAndBindingNumberByString( parameterBlockName, bindingName, m_hShader->GetShaderProgram()->getLayout() );

		if ( hSamplers.empty() )
		{
			MW_API_ERROR( "Passed invalid Sampler reference." );
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
		bool rewrite = forceRewrite || !m_BindingsWritten[{parameterBlock, descriptorSlot}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_pDescriptors[i].size() <= parameterBlock )
				m_pDescriptors[i].resize( parameterBlock + 1 );

			if ( !hSamplers[i] )
			{
				MW_API_ERROR( "Passed invalid Sampler reference." );
				return;
			}

			if ( m_pDescriptors[i][parameterBlock] == nullptr )
			{
				auto reflectionData = m_hShader->ReflectOnShader();
				if ( !reflectionData.pDescriptorSignatures[parameterBlock] )
				{
					MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", parameterBlock );
					return;
				}

				m_pDescriptors[i][parameterBlock] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[parameterBlock] );
			}

			if ( rewrite )
			{
				CDescriptorManager::WriteSampler( m_pDescriptors[i][parameterBlock], descriptorSlot, hSamplers[i] );

				m_BindingsWritten[{parameterBlock, descriptorSlot}] = true;
			}
		}
	};


	 void CDefferedFrameGraph::SubmitSceneGeometry( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		static float s_Angle = 0.0f;
		s_Angle = std::fmod( s_Angle + 0.007f, glm::two_pi<float>() );
		glm::mat4 transform = glm::rotate( glm::mat4( 1.0f ), s_Angle, glm::vec3( 1.0f, 1.0f, 0.0f ) );

		// Material / GBuffer draws need u_CameraConstants (set 1).
		CStaticRenderer::DrawStaticMeshIndexed( frameIndex, mesh, transform, -1 );

	}

	 void CDefferedFrameGraph::SubmitDepthPrePassGeometry( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !mesh || m_hGraphicsPrePasses.empty() || !m_hGraphicsPrePasses[0]->m_hGraphicsPipeline )
			return;

		static float s_Angle = 0.0f;
		s_Angle = std::fmod( s_Angle + 20.0f, glm::two_pi<float>() );
		glm::mat4 transform = glm::rotate( glm::mat4( 1.0f ), s_Angle, glm::vec3( 0.0f, 1.0f, 0.0f ) );

		CStaticRenderer::DrawStaticMeshIndexed(
			frameIndex,
			mesh,
			transform,
			-1,
			m_hGraphicsPrePasses[0]->m_hGraphicsPipeline );
	}

	CDefferedFrameGraph::CDefferedFrameGraph( Ref<CCamera> hCamera )	 NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_INFO( "Instantiate CDefferedFrameGraph" );
	
		m_hCamera = hCamera;

		ShaderCreateInfo shaderInfo{};
		shaderInfo.Flags = MW_SHADER_FLAG_INTERNAL_SLANG_SESSION;
		shaderInfo.Path = "shaders/DefaultMaterialStatic.slang"; // TODO: not hard code this

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

		// Set default vertex buffer layout to be used by every Vertex Shader.
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

		constexpr char tempEntry[] = "main"; // TODO: Remove

		SShaderObject vertexShader;
		vertexShader.ShaderStage = MW_SHADER_STAGE_VERTEX;
		vertexShader.pEntrypoint = tempEntry;

		auto program = m_hDefaultBasePassShader->GetShaderProgram();
		slang::ProgramLayout* layout = program->getLayout();

		// Get byte code for the Vertex Shader
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
						MW_ASSERT( false, "Failed to get vertex shader entry point code for default base pass: {}", static_cast< const char* >( diagnostics->getBufferPointer() ) );

					return;
				}

				vertexShader.Code = { code->getBufferPointer(), code->getBufferSize() };
			}
		}

		const char* fragmentEntrypoint = entrypoints[MW_SHADER_STAGE_FRAGMENT].c_str();

		SShaderObject pixelShader;
		pixelShader.ShaderStage = MW_SHADER_STAGE_FRAGMENT;
		pixelShader.pEntrypoint = tempEntry; 

		// Get byte code for the Pixel Shader
		{

			SlangInt pixelEntryPointIndex = -1;
			SlangInt entryPointCount = layout->getEntryPointCount();

			for ( SlangInt i = 0; i < entryPointCount; ++i )
			{
				slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );

				if ( entryPointLayout->getStage() == SLANG_STAGE_FRAGMENT )
				{
					pixelEntryPointIndex = i;
					break;
				}
			}

			if ( pixelEntryPointIndex != -1 )
			{
				Slang::ComPtr<slang::IBlob> code;
				Slang::ComPtr<slang::IBlob> diagnostics;
				const SlangInt targetIndex = 0;
				SlangResult result = program->getEntryPointCode(
					pixelEntryPointIndex,
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

				pixelShader.Code = { code->getBufferPointer(), code->getBufferSize() };

			}
		}

		// Add all shaders used by the base pass to an array to create the base pipeline.
		std::vector<SShaderObject> shaderObjects;
		shaderObjects.push_back( vertexShader );
		shaderObjects.push_back( pixelShader );

		// Add all color formats used in the gbuffer to an array also to create the base pipeline.
		std::vector<EImageFormat> colorFormats;
		colorFormats.emplace_back( MW_FORMAT_R8G8B8A8_UNORM ); // Albedo + Occlusion 
		colorFormats.emplace_back( MW_FORMAT_A2R10G10B10_UNORM_PACK32 ); // Normals, Roughness + Metallicness 
		colorFormats.emplace_back( MW_FORMAT_B10G11R11_UFLOAT_PACK32 ); // Emissive
		colorFormats.emplace_back( MW_FORMAT_R16G16_SFLOAT ); // Motion vectors
		colorFormats.emplace_back( MW_FORMAT_R32_UINT ); // Entity ID (bits 0-18) + Material ID (bits 19-31)

		// Color blending things also for creating the base pipeline.
		std::vector<SColorBlendAttachmentState> colorBlendAttachments;
		colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );
		colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );
		colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );
		colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );
		colorBlendAttachments.push_back( { MW_BLEND_MODE_NONE, false } );

		auto pipelineReflectData = m_hDefaultBasePassShader->ReflectOnShader();

		RHI::GraphicsPipelineCreationInfo createInfo{};
		createInfo.Flags = MW_PIPELINE_CREATION_FLAGS_DEFFERED_INITIALIZATION_BIT | MW_PIPELINE_CREATION_FLAGS_DISABLE_DEPTH_WRITE_BIT;
		createInfo.VertexLayout = basePassVertexLayout;
		createInfo.ShaderObjects = shaderObjects;
		createInfo.ColorFormats = colorFormats;
		createInfo.ColorBlendAttachments = colorBlendAttachments;
		createInfo.pSignature = pipelineReflectData.pPipelineSignature;
		createInfo.CompareOp = RHI::MW_COMPARE_OP_EQUAL;
		createInfo.DepthAttachmentFormat = MW_FORMAT_D32_SFLOAT;

		// Create the base pass pipeline.
		auto basePassPipeline = RHI::CPipelineManager::CreateGraphicsPipeline( &createInfo, &m_DefaultBasePassPipelineHash );

		if ( basePassPipeline )
			m_hDefaultBasePassPipeline = std::move(basePassPipeline.value());
		else
			MW_FATAL( "Failed to create base pass pipeline." );
		// TODO: Implement error handling here.
		

		auto re2d = CStaticRenderer::GetRenderableExtend();
		SExtent3D re = { re2d.Width, re2d.Height, 1 };

		// Fill the gbuffer for each frame in flight;
		// NOTE: There is a single gbuffer per frame in flight, not multiple like some might assume here
		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			auto& gbuf = m_hGBuffers[i];
			gbuf = Ref<GBuffer>::Create();

			constexpr auto commonFlags = MW_IMAGE_USAGE_SAMPLED_BIT;

			RHI::STextureCreateInfo gbufSamplerInfo{};
			gbufSamplerInfo.Extent = re;
			gbufSamplerInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			gbufSamplerInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			gbufSamplerInfo.Flags = MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_CREATION_BIT | MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_VIEW_CREATION_BIT;
			gbuf->Sampler = ITexture2D::Create( &gbufSamplerInfo );

			RHI::STextureCreateInfo gbufImgInfo{};
			gbufImgInfo.Extent = re;
			gbufImgInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			gbufImgInfo.Flags = MW_TEXTURE_CREATION_FLAG_DISABLE_SAMPLER_CREATION_BIT; // Not needed because we have a extra sampler.
			gbufImgInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | commonFlags;

			gbufImgInfo.Format = MW_FORMAT_A2R10G10B10_UNORM_PACK32;
			gbuf->NormalRoughMetal = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_B10G11R11_UFLOAT_PACK32;
			gbuf->Emissive = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_R16G16_SFLOAT;
			gbuf->MotionVector = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_R32_UINT;
			gbuf->EntityMaterialID = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_R8G8B8A8_UNORM;
			gbufImgInfo.ImageLayout = MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			gbufImgInfo.Usage |= MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | MW_IMAGE_USAGE_STORAGE_BIT;
			gbuf->AlbedoOcclusion = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_D32_SFLOAT;
			gbufImgInfo.ImageLayout = MW_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			gbufImgInfo.Usage = MW_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | commonFlags;
			gbufImgInfo.AspectMask = MW_IMAGE_ASPECT_DEPTH_BIT;
			gbuf->Depth = ITexture2D::Create( &gbufImgInfo );

			auto& composite = m_hCompositeImages[i];

			RHI::STextureCreateInfo compositeImgInfo{};
			compositeImgInfo.Extent = re;
			compositeImgInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			compositeImgInfo.Format = MW_FORMAT_R16G16B16A16_SFLOAT;
			compositeImgInfo.ImageLayout = MW_IMAGE_LAYOUT_GENERAL;
			compositeImgInfo.Usage = MW_IMAGE_USAGE_STORAGE_BIT | MW_IMAGE_USAGE_SAMPLED_BIT;
			composite = ITexture2D::Create( &compositeImgInfo );

			RHI::STextureCreateInfo presentationProxyImgInfo{};
			presentationProxyImgInfo.Extent = re;
			presentationProxyImgInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			presentationProxyImgInfo.Format = MW_FORMAT_B8G8R8A8_UNORM;
			presentationProxyImgInfo.ImageLayout = MW_IMAGE_LAYOUT_GENERAL;
			presentationProxyImgInfo.Usage = MW_IMAGE_USAGE_STORAGE_BIT | MW_IMAGE_USAGE_SAMPLED_BIT | MW_IMAGE_USAGE_TRANSFER_SRC_BIT;

			m_hPresentationProxyImages[i] = ITexture2D::Create(&presentationProxyImgInfo );

			// Root graphics CBs are not recording during frame-graph construction; use the uploader.
			m_hPresentationProxyImages[i]->TransitionLayout( i, MW_IMAGE_LAYOUT_GENERAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );


			gbuf->AlbedoOcclusion->TransitionLayout(	i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->NormalRoughMetal->TransitionLayout(	i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->Emissive->TransitionLayout(			i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->MotionVector->TransitionLayout(		i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->EntityMaterialID->TransitionLayout(	i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->Depth->TransitionLayout(				i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_DEPTH_BIT, -1, true );
		}

		// Create depth pre pass
		// TODO: Move somewhere else (move to the serialization of frame graphs)
		Ref<CGraphicsRenderPass> depthPrePass;
		{
			ShaderCreateInfo shaderInfo{};
			shaderInfo.Path = "shaders/DepthPrePass.slang";

			GraphicsRenderPassCreationInfo depthPrePassInfo{};
			depthPrePassInfo.hShader = Ref<CShader>::Create( &shaderInfo );

			std::array<RenderingAttachmentInfo, MFIF> depthAttachment;
			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				RenderingAttachmentInfo info{};
				info.hAttachmentImage = m_hGBuffers[i]->Depth; 

				depthAttachment[i] = info;
			}

			depthPrePassInfo.pDepthAttachment = depthAttachment;
			depthPrePassInfo.RenderingArea = re2d;
			// TODO: add thread ID
			depthPrePassInfo.Flags |= MW_GRAPHICS_PRE_PASS_CREATION_FLAGS_DISABLE_PIXEL_SHADER_BIT;

			depthPrePassInfo.pExecutionScopeCallback = [&]( u32 frameIndex )
				{
					SubmitDepthPrePassGeometry( frameIndex );
				};

			try 
			{
				depthPrePass = Ref<CGraphicsRenderPass>::Create( &depthPrePassInfo );
			}
			catch ( const std::runtime_error& e )
			{
				MW_ERROR( "Failed to create depth pre pass: {}", e.what() );
				MW_DEBUG_BREAK;
			}


		}

		// Create light pass 
		Ref<CComputeRenderPass> lightPass;
		{
			ShaderCreateInfo lightPassShader{};
			lightPassShader.Path = "shaders/LightingPass.slang";

			ComputeRenderPassCreationInfo lightPassInfo;
			lightPassInfo.hShader = Ref<CShader>::Create( &lightPassShader );

			lightPass = Ref<CComputeRenderPass>::Create( &lightPassInfo );

		}

		{
			ShaderCreateInfo tonemapShader{};
			tonemapShader.Path = "shaders/ToneMappingPass.slang";

			ComputeRenderPassCreationInfo tonemapPassInfo;
			tonemapPassInfo.hShader = Ref<CShader>::Create( &tonemapShader );

			try
			{
				m_hTonemapPass = Ref<CComputeRenderPass>::Create( &tonemapPassInfo );
			} 
			catch ( const std::runtime_error& e )
			{
				MW_ERROR( "Failed to create tonemapping pass: {}", e.what() );
				MW_DEBUG_BREAK;
			}
			// Read from cvars / config
			TonemapParams defaultParams{};
			defaultParams.Mode = MW_TONEMAP_MODE_NONE;
			defaultParams.Exposure = 1.0f;
			defaultParams.EnableGamma = false;

			m_hTonemapParamsUBO = IUniformBuffer::Create( sizeof( TonemapParams ) );
			m_hTonemapParamsUBO->SetData( &defaultParams, sizeof( TonemapParams ) );

			m_hTonemapPass->BindUBO( "u_Params", m_hTonemapParamsUBO, true );
			BindCommonData( m_hTonemapPass );
		}
		
		auto& lightShader = lightPass->m_hShader;
		auto lightReflectionData = lightShader->ReflectOnShader();

		auto depthReflectionData = depthPrePass->m_hShader->ReflectOnShader();
		CameraConstantsUBO cameraUBO;
		cameraUBO.CameraPosition = { 12.0f, 1.0f, 11.0f};
		cameraUBO.CurrentInverseViewProjection = glm::inverse( hCamera->GetCurrentViewProjectionMatrix() );
		cameraUBO.CurrentViewProjection = hCamera->GetCurrentViewProjectionMatrix();
		cameraUBO.PreviousInverseViewProjection = glm::inverse( hCamera->GetPreviousViewProjectionMatrix() );
		cameraUBO.PreviousViewProjection = hCamera->GetPreviousProjectionMatrix();

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			m_hCameraUBOs[i] = IUniformBuffer::Create( sizeof( CameraConstantsUBO ) );
			m_hCameraUBOs[i]->SetData( &cameraUBO, sizeof cameraUBO );
		}

		static std::array<SVertex, 24> s_CubeVertices =
		{ {
				// +X
				{ { 0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, 0 } },
				{ { 0.5f,  0.5f, -0.5f }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0 } },
				{ { 0.5f,  0.5f,  0.5f }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 1 } },
				{ { 0.5f, -0.5f,  0.5f }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1 } },

				// -X
				{ { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 0 } },
				{ { -0.5f, -0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 1, 0 } },
				{ { -0.5f,  0.5f,  0.5f }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 1, 1 } },
				{ { -0.5f,  0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1 } },

				// +Y
				{ { -0.5f, 0.5f, -0.5f }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 0 } },
				{ { -0.5f, 0.5f,  0.5f }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0 } },
				{ {  0.5f, 0.5f,  0.5f }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 1 } },
				{ {  0.5f, 0.5f, -0.5f }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1 } },

				// -Y
				{ { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 0 } },
				{ {  0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0 } },
				{ {  0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 1 } },
				{ { -0.5f, -0.5f,  0.5f }, { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1 } },

				// +Z
				{ { -0.5f, -0.5f, 0.5f }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0 } },
				{ {  0.5f, -0.5f, 0.5f }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 0 } },
				{ {  0.5f,  0.5f, 0.5f }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1 } },
				{ { -0.5f,  0.5f, 0.5f }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 1 } },

				// -Z
				{ { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0 } },
				{ { -0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 0 } },
				{ {  0.5f,  0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1 } },
				{ {  0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 1 } },
			} };

		static std::array<uint32_t, 36> s_CubeIndices =
		{
			 0,  1,  2,   0,  2,  3,  // +X
			 4,  5,  6,   4,  6,  7,  // -X
			 8,  9, 10,   8, 10, 11,  // +Y
			12, 13, 14,  12, 14, 15,  // -Y
			16, 17, 18,  16, 18, 19,  // +Z
			20, 21, 22,  20, 22, 23,  // -Z
		};

		mesh = Ref<CMesh>::Create();

		mesh->VertexBuffer = RHI::IVertexBuffer::Create( static_cast< void* >( s_CubeVertices.data() ), s_CubeVertices.size(), sizeof( SVertex ), true );
		mesh->IndexBuffer = RHI::IIndexBuffer::Create( s_CubeIndices.data(), s_CubeIndices.size(), 0, true );

		m_hTonemapPass->BindTexture( "u_ProxyImage", m_hPresentationProxyImages, true, MW_DESCRIPTOR_TYPE_STORAGE_IMAGE );

		depthPrePass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, true );
		lightPass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, true );

		this->AddPrePass( std::move(depthPrePass) );
		this->AddDefferedResolutionPass( std::move( lightPass ), 0 );

		CEventManager::Subscribe( MW_EVENT_WINDOW_RESIZE, [&]( SEvent& e ) { return OnResize( e ); } );

	};

	CDefferedFrameGraph::~CDefferedFrameGraph()	 NOEXCEPT 
	{
		MW_PROFILE_FUNC;
		m_hComputePrePasses.clear();
		m_hGraphicsPrePasses.clear();
		m_hDefferedResolutionPasses.clear();
		m_hPostProcessPasses.clear();

		CPipelineManager::DeleteGraphicsPipeline( m_DefaultBasePassPipelineHash );


		MW_INFO( "Destroy CDefferedFrameGraph " );
	};

	inline void CDefferedFrameGraph::BindCommonData( Ref<CComputeRenderPass>& hComputeRenderPass )
	{
		MW_PROFILE_FUNC;
		
		if ( HasParameterBlock( c_GbufBindingName, hComputeRenderPass->m_hShader->GetShaderProgram()->getLayout()) )
		{
			std::array<Ref<RHI::ITexture2D>, MFIF> ao;
			std::array<Ref<RHI::ITexture2D>, MFIF> nrm;
			std::array<Ref<RHI::ITexture2D>, MFIF> e;
			std::array<Ref<RHI::ITexture2D>, MFIF> mv;
			std::array<Ref<RHI::ITexture2D>, MFIF> d;
			std::array<Ref<RHI::ITexture2D>, MFIF> c;
			std::array<Ref<RHI::ITexture2D>, MFIF> s;

			for ( auto i{ 0uz }; i < MFIF; i++ )
			{
				if ( !ao[i] )
					ao[i] = m_hGBuffers[i]->AlbedoOcclusion;

				if ( !nrm[i] )
					nrm[i] = m_hGBuffers[i]->NormalRoughMetal;

				if ( !e[i] )
					e[i] = m_hGBuffers[i]->Emissive;

				if ( !mv[i] )
					mv[i] = m_hGBuffers[i]->MotionVector;

				if ( !d[i] )
					d[i] = m_hGBuffers[i]->Depth;

				if ( !s[i] )
					s[i] = m_hGBuffers[i]->Sampler;

				if ( !c[i] )
					c[i] = m_hCompositeImages[i];
			}

			hComputeRenderPass->BindTexture( c_GbufBindingName, c_GbufAOBindingName, ao );
			hComputeRenderPass->BindTexture( c_GbufBindingName, c_GbufNRMBindingName, nrm );
			hComputeRenderPass->BindTexture( c_GbufBindingName, c_GbufEBindingName, e );
			hComputeRenderPass->BindTexture( c_GbufBindingName, c_GbufMVBindingName, mv );
			hComputeRenderPass->BindTexture( c_GbufBindingName, c_GbufDBindingName, d );
			hComputeRenderPass->BindTexture( c_GbufBindingName, c_GbufCBindingName, c, false, MW_DESCRIPTOR_TYPE_STORAGE_IMAGE );
			hComputeRenderPass->BindSampler( c_GbufBindingName, c_GbufSBindingName, s );
		}

		if ( HasGlobalBinding( c_CameraConstantsBindingName, hComputeRenderPass->m_hShader->GetShaderProgram()->getLayout() ) )
			hComputeRenderPass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, false);
	}

	bool MWAPI_CALL CDefferedFrameGraph::OnResize( SEvent& e )
	{
		MW_PROFILE_FUNC;
		CStaticRenderer::WaitRenderingIdle();
		MW_INFO("Recreating GBuffers.");
		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			auto& gbuf = m_hGBuffers[i];
			gbuf = Ref<GBuffer>::Create();

			auto extent = e.GetPayload<Events::SWindowResize>().NewExtent;

			SExtent3D re = { extent.Width, extent.Height, 1 }; 

			constexpr auto commonFlags = MW_IMAGE_USAGE_SAMPLED_BIT;

			RHI::STextureCreateInfo gbufSamplerInfo{};
			gbufSamplerInfo.Extent = re;
			gbufSamplerInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			gbufSamplerInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			gbufSamplerInfo.Flags = MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_CREATION_BIT | MW_TEXTURE_CREATION_FLAG_DISABLE_IMAGE_VIEW_CREATION_BIT;
			gbuf->Sampler = ITexture2D::Create( &gbufSamplerInfo );

			RHI::STextureCreateInfo gbufImgInfo{};
			gbufImgInfo.Extent = re;
			gbufImgInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			gbufImgInfo.Flags = MW_TEXTURE_CREATION_FLAG_DISABLE_SAMPLER_CREATION_BIT; // Not needed because we have a extra sampler.
			gbufImgInfo.Usage = MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | commonFlags;

			gbufImgInfo.Format = MW_FORMAT_A2R10G10B10_UNORM_PACK32;
			gbuf->NormalRoughMetal = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_B10G11R11_UFLOAT_PACK32;
			gbuf->Emissive = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_R16G16_SFLOAT;
			gbuf->MotionVector = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_R32_UINT;
			gbuf->EntityMaterialID = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_R8G8B8A8_UNORM;
			gbufImgInfo.ImageLayout = MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			gbufImgInfo.Usage |= MW_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | MW_IMAGE_USAGE_STORAGE_BIT;
			gbuf->AlbedoOcclusion = ITexture2D::Create( &gbufImgInfo );

			gbufImgInfo.Format = MW_FORMAT_D32_SFLOAT;
			gbufImgInfo.ImageLayout = MW_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			gbufImgInfo.Usage = MW_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | commonFlags;
			gbufImgInfo.AspectMask = MW_IMAGE_ASPECT_DEPTH_BIT;
			gbuf->Depth = ITexture2D::Create( &gbufImgInfo );

			auto& composite = m_hCompositeImages[i];

			RHI::STextureCreateInfo compositeImgInfo{};
			compositeImgInfo.Extent = re;
			compositeImgInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			compositeImgInfo.Format = MW_FORMAT_R16G16B16A16_SFLOAT;
			compositeImgInfo.ImageLayout = MW_IMAGE_LAYOUT_GENERAL;
			compositeImgInfo.Usage = MW_IMAGE_USAGE_STORAGE_BIT | MW_IMAGE_USAGE_SAMPLED_BIT;
			composite = ITexture2D::Create( &compositeImgInfo );

			RHI::STextureCreateInfo presentationProxyImgInfo{};
			presentationProxyImgInfo.Extent = re;
			presentationProxyImgInfo.AspectMask = MW_IMAGE_ASPECT_COLOR_BIT;
			presentationProxyImgInfo.Format = MW_FORMAT_B8G8R8A8_UNORM;
			presentationProxyImgInfo.ImageLayout = MW_IMAGE_LAYOUT_GENERAL;
			presentationProxyImgInfo.Usage = MW_IMAGE_USAGE_STORAGE_BIT | MW_IMAGE_USAGE_SAMPLED_BIT | MW_IMAGE_USAGE_TRANSFER_SRC_BIT;

			m_hPresentationProxyImages[i] = ITexture2D::Create( &presentationProxyImgInfo );

			// Root graphics CBs are not recording during frame-graph construction; use the uploader.
			m_hPresentationProxyImages[i]->TransitionLayout( i, MW_IMAGE_LAYOUT_GENERAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );


			gbuf->AlbedoOcclusion->TransitionLayout( i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->NormalRoughMetal->TransitionLayout( i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->Emissive->TransitionLayout( i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->MotionVector->TransitionLayout( i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->EntityMaterialID->TransitionLayout( i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_COLOR_BIT, -1, true );
			gbuf->Depth->TransitionLayout( i, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_ALL_COMMANDS_BIT, MW_IMAGE_ASPECT_DEPTH_BIT, -1, true );
		}


		for ( auto& hDefferedResolutionPass : m_hDefferedResolutionPasses )
		{
			if ( !hDefferedResolutionPass )
				continue;
			
			if ( HasParameterBlock( c_GbufBindingName, hDefferedResolutionPass->m_hShader->GetShaderProgram()->getLayout() ) )
			{
				std::array<Ref<RHI::ITexture2D>, MFIF> ao;
				std::array<Ref<RHI::ITexture2D>, MFIF> nrm;
				std::array<Ref<RHI::ITexture2D>, MFIF> e;
				std::array<Ref<RHI::ITexture2D>, MFIF> mv;
				std::array<Ref<RHI::ITexture2D>, MFIF> d;
				std::array<Ref<RHI::ITexture2D>, MFIF> c;
				std::array<Ref<RHI::ITexture2D>, MFIF> s;

				for ( auto i{ 0uz }; i < MFIF; i++ )
				{
					if ( !ao[i] )
						ao[i] = m_hGBuffers[i]->AlbedoOcclusion;

					if ( !nrm[i] )
						nrm[i] = m_hGBuffers[i]->NormalRoughMetal;

					if ( !e[i] )
						e[i] = m_hGBuffers[i]->Emissive;

					if ( !mv[i] )
						mv[i] = m_hGBuffers[i]->MotionVector;

					if ( !d[i] )
						d[i] = m_hGBuffers[i]->Depth;

					if ( !s[i] )
						s[i] = m_hGBuffers[i]->Sampler;

					if ( !c[i] )
						c[i] = m_hCompositeImages[i];
				}

				hDefferedResolutionPass->BindTexture( c_GbufBindingName, c_GbufAOBindingName, ao );
				hDefferedResolutionPass->BindTexture( c_GbufBindingName, c_GbufNRMBindingName, nrm );
				hDefferedResolutionPass->BindTexture( c_GbufBindingName, c_GbufEBindingName, e );
				hDefferedResolutionPass->BindTexture( c_GbufBindingName, c_GbufMVBindingName, mv );
				hDefferedResolutionPass->BindTexture( c_GbufBindingName, c_GbufDBindingName, d );
				hDefferedResolutionPass->BindTexture( c_GbufBindingName, c_GbufCBindingName, c, false, MW_DESCRIPTOR_TYPE_STORAGE_IMAGE );
				hDefferedResolutionPass->BindSampler( c_GbufBindingName, c_GbufSBindingName, s );
			}

			if ( HasGlobalBinding( c_CameraConstantsBindingName, hDefferedResolutionPass->m_hShader->GetShaderProgram()->getLayout() ) )
				hDefferedResolutionPass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, false );
		}

		for ( auto& hPostProcessPass : m_hPostProcessPasses )
		{
			if ( !hPostProcessPass )
				continue;

			if ( HasParameterBlock( c_GbufBindingName, hPostProcessPass->m_hShader->GetShaderProgram()->getLayout() ) )
			{
				std::array<Ref<RHI::ITexture2D>, MFIF> ao;
				std::array<Ref<RHI::ITexture2D>, MFIF> nrm;
				std::array<Ref<RHI::ITexture2D>, MFIF> e;
				std::array<Ref<RHI::ITexture2D>, MFIF> mv;
				std::array<Ref<RHI::ITexture2D>, MFIF> d;
				std::array<Ref<RHI::ITexture2D>, MFIF> c;
				std::array<Ref<RHI::ITexture2D>, MFIF> s;

				for ( auto i{ 0uz }; i < MFIF; i++ )
				{
					if ( !ao[i] )
						ao[i] = m_hGBuffers[i]->AlbedoOcclusion;

					if ( !nrm[i] )
						nrm[i] = m_hGBuffers[i]->NormalRoughMetal;

					if ( !e[i] )
						e[i] = m_hGBuffers[i]->Emissive;

					if ( !mv[i] )
						mv[i] = m_hGBuffers[i]->MotionVector;

					if ( !d[i] )
						d[i] = m_hGBuffers[i]->Depth;

					if ( !s[i] )
						s[i] = m_hGBuffers[i]->Sampler;

					if ( !c[i] )
						c[i] = m_hCompositeImages[i];
				}

				hPostProcessPass->BindTexture( c_GbufBindingName, c_GbufAOBindingName, ao );
				hPostProcessPass->BindTexture( c_GbufBindingName, c_GbufNRMBindingName, nrm );
				hPostProcessPass->BindTexture( c_GbufBindingName, c_GbufEBindingName, e );
				hPostProcessPass->BindTexture( c_GbufBindingName, c_GbufMVBindingName, mv );
				hPostProcessPass->BindTexture( c_GbufBindingName, c_GbufDBindingName, d );
				hPostProcessPass->BindTexture( c_GbufBindingName, c_GbufCBindingName, c, false, MW_DESCRIPTOR_TYPE_STORAGE_IMAGE );
				hPostProcessPass->BindSampler( c_GbufBindingName, c_GbufSBindingName, s );
			}

			if ( HasGlobalBinding( c_CameraConstantsBindingName, hPostProcessPass->m_hShader->GetShaderProgram()->getLayout() ) )
				hPostProcessPass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, false );
		}
		return false; 
	}

	void CDefferedFrameGraph::AddPrePass( Ref<CComputeRenderPass> hComputePrePass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( !hComputePrePass )
		{
			MW_API_WARN( "Passed invalid hComputePrePass to CDefferedFrameGraph::AddPrePass (Compute). Discarding.");
			return;
		}
		
		if ( HasGlobalBinding( c_CameraConstantsBindingName, hComputePrePass->m_hShader->GetShaderProgram()->getLayout() ) )
			hComputePrePass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, false );

		if ( executionPriority < 0 || executionPriority >=  m_hComputePrePasses.size() )
		{
			MW_INFO( "Register Compute Pre-Pass {} at execution priority {}", static_cast<void*>(hComputePrePass.raw()), m_hComputePrePasses.size() );
			m_hComputePrePasses.push_back( std::move( hComputePrePass ) );
		}
		else
		{
			MW_INFO( "Register Compute Pre-Pass {} by inserting it at execution priority {}", static_cast< void* >( hComputePrePass.raw()), executionPriority );
			m_hComputePrePasses.insert( m_hComputePrePasses.begin() + executionPriority, std::move( hComputePrePass ) );
		}
	};

	void CDefferedFrameGraph::AddPrePass( Ref<CGraphicsRenderPass> hGraphicsPrePass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		if ( !hGraphicsPrePass )
		{
			MW_API_WARN( "Passed invalid hGraphicsPrePass to CDefferedFrameGraph::AddPrePass (Graphics). Discarding." );
			return;
		}

		if ( HasGlobalBinding( c_CameraConstantsBindingName, hGraphicsPrePass->m_hShader->GetShaderProgram()->getLayout() ) )
			hGraphicsPrePass->BindUBO( c_CameraConstantsBindingName, m_hCameraUBOs, false );

		if ( executionPriority < 0 || executionPriority >= m_hGraphicsPrePasses.size() )
		{
			MW_INFO( "Register Graphics Pre-Pass {} at execution priority {}", static_cast< void* >( hGraphicsPrePass.raw() ), m_hGraphicsPrePasses.size() );
			m_hGraphicsPrePasses.push_back( std::move( hGraphicsPrePass ) );
		}
		else
		{
			MW_INFO( "Register Graphics Pre-Pass {} by inserting it at execution priority {}", static_cast< void* >( hGraphicsPrePass.raw() ), executionPriority );
			m_hGraphicsPrePasses.insert( m_hGraphicsPrePasses.begin() + executionPriority, std::move( hGraphicsPrePass ) );
		}
	};

	void CDefferedFrameGraph::AddDefferedResolutionPass( Ref<CComputeRenderPass> hComputePass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hComputePass )
		{
			MW_API_WARN( "Passed invalid hComputePass to CDefferedFrameGraph::AddDefferedResolutionPass. Discarding." );
			return;
		}

		BindCommonData( hComputePass );

		if ( executionPriority < 0 || executionPriority >=  m_hDefferedResolutionPasses.size() )
		{
			MW_INFO( "Register Deffered Resolution Pass {} at execution priority {}", static_cast< void* >( hComputePass.raw() ), m_hDefferedResolutionPasses.size() );
			m_hDefferedResolutionPasses.push_back( std::move( hComputePass ) );
		}
		else
		{
			MW_INFO( "Register Deffered Resolution Pass {} by inserting it at execution priority {}", static_cast< void* >( hComputePass.raw() ), executionPriority );
			m_hDefferedResolutionPasses.insert( m_hDefferedResolutionPasses.begin() + executionPriority, std::move( hComputePass ) );
		}
	};

	void CDefferedFrameGraph::AddPostProcessPass( Ref<CComputeRenderPass> hPostProcessPass, s32 MW_NULLABLE executionPriority ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hPostProcessPass )
		{
			MW_API_WARN( "Passed invalid hPostProcessPass to CDefferedFrameGraph::AddPostProcessPass. Discarding." );
			return;
		}

		BindCommonData( hPostProcessPass );

		// Check if either the execution priority would be bi
		if ( executionPriority < 0 || executionPriority >= m_hPostProcessPasses.size() )
		{
			// Insert the post process pass last.
			MW_INFO( "Register Post Processing Pass {} at execution priority {}", static_cast< void* >( hPostProcessPass.raw()), m_hPostProcessPasses.size() );
			m_hPostProcessPasses.push_back( std::move( hPostProcessPass ) );
		}
		else
		{
			// Insert the post process pass at the execution priority and shifting all other passes back one unit.
			MW_INFO( "Register Post Processing Pass {} by inserting it at execution priority {}", static_cast< void* >( hPostProcessPass.raw() ), executionPriority );
			m_hPostProcessPasses.insert( m_hPostProcessPasses.begin() + executionPriority, std::move( hPostProcessPass ) );
		}
	};


	static void  BindCommandBufferStateBothBegun( u32 frameIndex )
	{
		MW_PROFILE_FUNC;

		auto ex = CStaticRenderer::GetRenderableExtend();

		CStaticRenderer::SetDynamicCullMode( frameIndex, MW_CULL_MODE_BACK );
		CStaticRenderer::SetDynamicScissors( frameIndex, { &ex, 1 }, 0 );

		Viewport viewport{};
		viewport.Width = ( float )ex.Width;
		viewport.Height = ( float )ex.Height;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;

		CStaticRenderer::SetDynamicViewports( frameIndex, { &viewport, 1 }, 0 );

	}

	// Binds static dynamic state only to the root command buffer. Use this if the worker commandbuffers have not started yet.
	static void  BindCommandBufferStateOnlyRoot( u32 frameIndex )
	{
		MW_PROFILE_FUNC;

		auto ex = CStaticRenderer::GetRenderableExtend();

		CStaticRenderer::SetDynamicCullModeST( frameIndex, MW_CULL_MODE_BACK, -1 );
		CStaticRenderer::SetDynamicScissorsST( frameIndex, { &ex, 1 }, 0, -1 );

		Viewport viewport{};
		viewport.Width = ( float )ex.Width;
		viewport.Height = ( float )ex.Height;
		viewport.X = 0;
		viewport.Y = 0;
		viewport.MaxDepth = 1.0f;
		viewport.MinDepth = 0.0f;

		CStaticRenderer::SetDynamicViewportsST( frameIndex, { &viewport, 1 }, 0, -1 );

	}

	 void CDefferedFrameGraph::ExecutePreRenderingSteps( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !mesh->Material )
		{
			MaterialCreationInfo matCreateInfo{};
			matCreateInfo.AlbedoFactor = { 1.0f, 1.0f, 0.0f };
			matCreateInfo.hAlbedoMap = CStaticRenderer::GetDefaultMissingMap();

			Ref<CMaterial> mat = Ref<CMaterial>::Create( &matCreateInfo );

			mesh->Material = std::move( mat );
		}

		auto& gbuf = m_hGBuffers[frameIndex];

		gbuf->AlbedoOcclusion->TransitionLayout( frameIndex,	MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->NormalRoughMetal->TransitionLayout( frameIndex,	MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->Emissive->TransitionLayout( frameIndex,			MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->MotionVector->TransitionLayout( frameIndex,		MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->EntityMaterialID->TransitionLayout( frameIndex,	MW_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, MW_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->Depth->TransitionLayout( frameIndex,				MW_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, MW_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, MW_IMAGE_ASPECT_DEPTH_BIT );

		m_hCompositeImages[frameIndex]->TransitionLayout( frameIndex, MW_IMAGE_LAYOUT_GENERAL, MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );

		// TODO: add check if m_CameraUBOs[ frameIndex ] is valid
		CameraConstantsUBO ubo{};
		ubo.CameraPosition = { 0, 0, 4.2 };
		ubo.CurrentViewProjection = m_hCamera->GetCurrentViewProjectionMatrix();
		ubo.PreviousViewProjection = m_hCamera->GetPreviousViewProjectionMatrix();
		ubo.CurrentInverseViewProjection = glm::inverse( m_hCamera->GetCurrentViewProjectionMatrix() );
		ubo.PreviousInverseViewProjection = glm::inverse( m_hCamera->GetPreviousViewProjectionMatrix() );

		m_hCameraUBOs[frameIndex]->SetData( &ubo, sizeof( ubo ) );

		// Root commandbuffer recording has already begun at this point. The
		// frame manager has already called CStaticRenderer::BeginRootCommandbuffer. 
		// That's because it should not be implementation specific 
		BindCommandBufferStateOnlyRoot( frameIndex );
	}

	void CDefferedFrameGraph::ExecutePrePasses( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		CStaticRenderer::BeginSecondaryCommandbuffers( frameIndex );
		BindCommandBufferStateBothBegun( frameIndex );

		// TODO: Convert this to a Job
		// TODO: cvars
		for ( auto& computePrePass : m_hComputePrePasses )
		{
			auto workgroup = ComputeWorkgroupSize( computePrePass->m_hShader->GetShaderProgram()->getLayout()->getEntryPointByIndex( 0 ) );
			CStaticRenderer::DispatchCompute( frameIndex, computePrePass->m_hComputePipeline, workgroup, computePrePass->m_pDescriptors[frameIndex], computePrePass->m_FirstSet );
		}

		// Merge secondaries because every graphics pre-pass must have their own secondaries. That's just the way it is I'm just the messenger
		CStaticRenderer::MergeSecondaryCommandbuffers( frameIndex ); 

		// TODO: Job
		for ( auto& graphicsPrePass : m_hGraphicsPrePasses )
		{
			if ( !graphicsPrePass ) continue;

			// TODO: Don't do this
			std::vector<RenderingAttachmentInfo> colorAttachments;
			if ( graphicsPrePass->m_ColorAttachments )
				colorAttachments = graphicsPrePass->m_ColorAttachments.value()[frameIndex];
				 

			RHI::BeginRenderingInfo info;
			info.Flags = MW_RENDERING_FLAGS_WITH_SECONDARY_COMMAND_BUFFERS_BIT;
			if ( graphicsPrePass->m_ColorAttachments )
				info.pColorAttachments = colorAttachments;
			if ( graphicsPrePass->m_DepthAttachment )
				info.pDepthAttachment = graphicsPrePass->m_DepthAttachment.value()[frameIndex];

			if ( graphicsPrePass->m_StencilAttachment )
				info.pStencilAttachment = graphicsPrePass->m_StencilAttachment.value()[frameIndex];

			info.RenderArea = graphicsPrePass->m_RenderingArea;
			
			CStaticRenderer::BeginSecondaryCommandbuffers( frameIndex );
			BindCommandBufferStateBothBegun( frameIndex );

			CStaticRenderer::BeginRendering( frameIndex, &info ); 

			// Bind the pre pass pipeline generated from graphicsPrePass::m_hShader and the descriptor for the shader globals.
			CStaticRenderer::BindGraphicsPipeline( frameIndex, graphicsPrePass->m_hGraphicsPipeline );

			// Maybe confusing but this only binds global scope descriptors, not any others. So only bind them if they actually exist
			CStaticRenderer::BindDescriptors( frameIndex, *graphicsPrePass->m_hGraphicsPipeline->GetSignature(), graphicsPrePass->m_pDescriptors[frameIndex], graphicsPrePass->m_FirstSet, -1, MW_PIPELINE_BIND_POINT_GRAPHICS );

			// Invoke the execution callback specified by the pre-pass. The pre-pass manager can submit draw calls, bind state etc. here.
			if ( graphicsPrePass->m_pExecutionScopeCallback )
				graphicsPrePass->m_pExecutionScopeCallback(frameIndex);

			// Merge before ending rendering, as specified by the Vulkan Specification. 
			CStaticRenderer::MergeSecondaryCommandbuffers( frameIndex );

			CStaticRenderer::EndRendering( frameIndex );
		}
	};

	void CDefferedFrameGraph::ExecuteBasePasses( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
 		
		CStaticRenderer::BeginSecondaryCommandbuffers( frameIndex );
		BindCommandBufferStateBothBegun( frameIndex );

		std::vector<RHI::RenderingAttachmentInfo> gbufferAttachments;
		constexpr int gbufferColorAttachmentCount = 5;
		gbufferAttachments.resize( gbufferColorAttachmentCount );

		gbufferAttachments[0] = { m_hGBuffers[frameIndex]->AlbedoOcclusion };
		gbufferAttachments[1] = { m_hGBuffers[frameIndex]->NormalRoughMetal };
		gbufferAttachments[2] = { m_hGBuffers[frameIndex]->Emissive };
		gbufferAttachments[3] = { m_hGBuffers[frameIndex]->MotionVector };
		gbufferAttachments[4] = { m_hGBuffers[frameIndex]->EntityMaterialID };
	
		RHI::RenderingAttachmentInfo depthAttachment;
		depthAttachment.hAttachmentImage = m_hGBuffers[frameIndex]->Depth;
		// Preserve depth written by the depth pre-pass for EQUAL depth testing in the base pass.
		depthAttachment.LoadOp = MW_ATTACHMENT_LOAD_OP_LOAD;

		RHI::BeginRenderingInfo renderingInfo;
		renderingInfo.Flags = MW_RENDERING_FLAGS_WITH_SECONDARY_COMMAND_BUFFERS_BIT;
		renderingInfo.pColorAttachments = gbufferAttachments;
		renderingInfo.pDepthAttachment = depthAttachment;
		renderingInfo.RenderArea = CStaticRenderer::GetRenderableExtend();

		CStaticRenderer::BeginRendering( frameIndex, &renderingInfo );

		// Jobs
		SubmitSceneGeometry( frameIndex );

		CStaticRenderer::MergeSecondaryCommandbuffers( frameIndex );
		CStaticRenderer::EndRendering( frameIndex );
				

		// Deffered Resolution Pass

		// Begin Secondary commandbuffers for the rest of the frame.
		// Merging happens at the end of the frame.

		auto gbuf = m_hGBuffers[frameIndex];

		gbuf->AlbedoOcclusion->TransitionLayout( frameIndex,	MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT,	MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->NormalRoughMetal->TransitionLayout( frameIndex,	MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT,	MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->Emissive->TransitionLayout( frameIndex, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->MotionVector->TransitionLayout( frameIndex, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->EntityMaterialID->TransitionLayout( frameIndex, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, MW_IMAGE_ASPECT_COLOR_BIT );
		gbuf->Depth->TransitionLayout( frameIndex, MW_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MW_PIPELINE_STAGE_COMPUTE_SHADER_BIT, MW_IMAGE_ASPECT_DEPTH_BIT );


		CStaticRenderer::BeginSecondaryCommandbuffers( frameIndex );
		BindCommandBufferStateBothBegun( frameIndex );

		for ( auto& resolutionPass : m_hDefferedResolutionPasses )
		{
			auto workgroup = ComputeWorkgroupSize( resolutionPass->m_hShader->GetShaderProgram()->getLayout()->getEntryPointByIndex( 0 ) );

			CStaticRenderer::DispatchCompute( frameIndex, resolutionPass->m_hComputePipeline, workgroup, resolutionPass->m_pDescriptors[frameIndex], resolutionPass->m_FirstSet );
		}
		
	};


	void CDefferedFrameGraph::ExecutePostPasses( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		auto dispatchPostCompute = [this]( u32 frameIndex, Ref<CComputeRenderPass>& pass )
			{
				auto workgroup = ComputeWorkgroupSize( pass->m_hShader->GetShaderProgram()->getLayout()->getEntryPointByIndex( 0 ) );

				CStaticRenderer::DispatchCompute( frameIndex, pass->m_hComputePipeline, workgroup, pass->m_pDescriptors[frameIndex], pass->m_FirstSet );
			};
		
		for ( auto& postPass : m_hPostProcessPasses )
		{
			// TODO: Jobs
			dispatchPostCompute( frameIndex, postPass );
		}
		// TODO: Add support for post and pre tone mapping passes

		// Execute Special Fixed Passes (Tone mapping)
		dispatchPostCompute( frameIndex, m_hTonemapPass );


	};

	void CDefferedFrameGraph::ExecutePostRenderingSteps( u32 frameIndex ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		CStaticRenderer::MergeSecondaryCommandbuffers( frameIndex );
		// root commandbuffer submission happens in the frame manager, as it's not 
		// implementation specific. 

		auto presenter = CApplication::GetCreateInfos()->pPresenter;
		m_hPresentationProxyImages[frameIndex]->CopyImage( frameIndex, presenter->GetSwapchainImages()[CFrameManager::GetCurrentImageIndex()] );
	};



}
