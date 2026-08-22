#include <common/Base.hh>

#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>

#include <rhi/agnostic/PipelineManager.hh>

#include "FrameGraph.hh"

namespace Monoworks 
{
	using namespace RHI;

	CDefferedResolutionPass::CDefferedResolutionPass( DefferedResolutionPassCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		
		if ( !pInfo->hShader )
		{
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
			pipeline = CPipelineManager::CreateComputePipeline( &pipelineInfo );
		}
		catch ( ... )
		{
			MW_WARN( "Failed to create Compute Pipeline for Deffered Resolution Pass " );
		}

		if ( pipeline )
			m_hComputePipeline = pipeline.value();
		



	};

	// 1. Pipeline Creation
	// 2. reflection -> descriptor layout creation
	// 3. descriptor set allocation
	// 4. Buffer & texture upload

	void CDefferedResolutionPass::BindTexture( std::string_view name, Ref<RHI::ITexture2D> hTexture )
	{
		MW_PROFILE_FUNC;

	};

	void CDefferedResolutionPass::BindUBO( std::string_view name, Ref<RHI::IUniformBuffer> hUniformBuffer ) 
	{
		MW_PROFILE_FUNC;

	};


}
