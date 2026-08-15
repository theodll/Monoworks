#include <mwpch.hh>

#include "Shader.hh"

#include <renderer/StaticRenderer.hh>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Monoworks 
{
	CShader::CShader( const ShaderCreateInfo* pInfo ) : m_Path( pInfo->Path ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_Ready = false; 

		m_CreateInfo = *pInfo;

		if ( ( pInfo->Flags & MW_SHADER_FLAG_INTERNAL_SLANG_SESSION ) )
		{
			auto globalSession = CStaticRenderer::GetSlangGlobalSession();
			
			slang::SessionDesc sessionDesc{};
			
			slang::TargetDesc targetDesc{};
			// TODO: change if using metal or direct3d
			targetDesc.format = SLANG_SPIRV;
			targetDesc.profile = globalSession->findProfile( "spirv_1_5" );

			sessionDesc.targets = &targetDesc;
			sessionDesc.targetCount = 1;

			sessionDesc.preprocessorMacros = pInfo->SlangPreprocessorMacros;
			sessionDesc.preprocessorMacroCount = pInfo->SlangPreprocessorMacroCount;

			std::array<slang::CompilerOptionEntry, 1> options =
			{
				{
					slang::CompilerOptionName::EmitSpirvDirectly,
					{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
				}
			};
			sessionDesc.compilerOptionEntries = options.data();
			sessionDesc.compilerOptionEntryCount = options.size();

			globalSession->createSession( sessionDesc, m_SlangSession.writeRef() );
		} 
		else 
		{
			if ( pInfo->SlangSession )
				m_SlangSession = pInfo->SlangSession;
		}

		if ( !( pInfo->Flags & MW_SHADER_FLAG_DEFFERED_COMPILATION_BIT ) )
		{
			CompileShader();
		}

		if ( !( pInfo->Flags & MW_SHADER_FLAG_DEFFERED_REFLECTION_BIT) )
		{
			ReflectOnShader();
		}

	}

	constexpr static std::string_view StripSlangExtension( std::string_view path )
	{
		constexpr std::string_view suffix = ".slang";
		if ( path.ends_with( suffix ) )
			return path.substr( 0, path.size() - suffix.size() );
		return path;
	};

	void CShader::CompileShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;


	}

	void CShader::ReflectOnShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}


}

