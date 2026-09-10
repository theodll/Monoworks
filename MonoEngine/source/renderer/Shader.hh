#pragma once
#include <common/Base.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>

#include <unordered_map>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Monoworks 
{
	enum EShaderFlagBits
	{
		MW_SHADER_FLAG_NONE_BIT = 0,
		MW_SHADER_FLAG_DEFFERED_COMPILATION_BIT = 0b1,
		MW_SHADER_FLAG_DEFFERED_REFLECTION_BIT = 0b10,
		MW_SHADER_FLAG_INTERNAL_SLANG_SESSION = 0b100,
		MW_SHADER_FLAG_MAX_ENUM = 0x7FFFFFFF
	};

	using EShaderFlags = flags_t;

	// TODO: Rename to ShaderCreationInfo
	struct ShaderCreateInfo 
	{
		path_t Path; // NOTE: Module name + .slang relative to the binaries directory 
		EShaderFlags Flags;
		
		Slang::ComPtr<slang::ISession> MW_NULLABLE SlangSession; // NOTE: null when MW_SHADER_FLAG_INTERNAL_SLAG_SESSION is disabled.
		
		char const* const* MW_NULLABLE SlangSessionSearchPaths;
		SlangInt SlangSessionSearchPathCount;
		
		slang::PreprocessorMacroDesc const* MW_NULLABLE SlangPreprocessorMacros;
		SlangInt SlangPreprocessorMacroCount;
	};

	struct PushConstantRanges 
	{
		uint32_t              Offset;
		uint32_t              Size;
	};
	
	struct ShaderReflectionData
	{
		RHI::PipelineSignature						pPipelineSignature; // void* 
		std::vector<RHI::DescriptorSignature>		pDescriptorSignatures; // void* 
		std::vector<PushConstantRanges> MW_NULLABLE PushConstantRanges;
	};

	class CShader
	{
	public:
		CShader( const ShaderCreateInfo* pInfo )	NOEXCEPT;

		void CompileShader() NOEXCEPT;
		ShaderReflectionData ReflectOnShader() NOEXCEPT; 

		NODISCARD const boost::unordered_map<RHI::EShaderStage, std::string>& GetShaderEntrypoints() NOEXCEPT { return m_Entrypoints; };
		NODISCARD Slang::ComPtr<slang::IComponentType> GetShaderProgram() NOEXCEPT { return m_pSlangProgram; }
	private:

		ShaderCreateInfo m_CreateInfo;
		boost::unordered_map<RHI::EShaderStage, std::string> m_Entrypoints;
		path_t m_Path;
		Slang::ComPtr<slang::ISession> m_pSlangSession;
		Slang::ComPtr<slang::IComponentType> m_pSlangProgram;
		bool m_Ready;
	};
}
