#pragma once
#include <common/Base.hh>

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

	// Shader entrypoint must be main. 
	struct ShaderCreateInfo 
	{
		path_t Path; // NOTE: Module name + .slang relative to the binaries directory 
		EShaderFlags Flags;
		
		Slang::ComPtr<slang::ISession> MW_NULLABLE SlangSession; // NOTE: null when MW_SHADER_FLAG_INTERNAL_SLAG_SESSION is disabled.
		
		const char const* const* MW_NULLABLE SlangSessionSearchPaths;
		SlangInt SlangSessionSearchPathCount;
		
		slang::PreprocessorMacroDesc const* MW_NULLABLE SlangPreprocessorMacros;
		SlangInt SlangPreprocessorMacroCount;
	};

	class CShader
	{
	public:
		CShader( const ShaderCreateInfo* pInfo )	NOEXCEPT;

		void CompileShader()	NOEXCEPT;
		void ReflectOnShader()	NOEXCEPT;

	private:
		ShaderCreateInfo m_CreateInfo;
		path_t m_Path;
		Slang::ComPtr<slang::ISession> m_SlangSession;
		Slang::ComPtr<slang::IBlob> m_SlangCompiledBinaryData;
		bool m_Ready;
	};
}
