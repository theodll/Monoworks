#include <mwpch.hh>

#include "Shader.hh"

#include <renderer/StaticRenderer.hh>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Monoworks 
{
	CShader::CShader( const ShaderCreateInfo* pInfo ) NOEXCEPT : m_Path( pInfo->Path )
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

			if ( pInfo->SlangPreprocessorMacros && pInfo->SlangPreprocessorMacroCount > 0 )
			{
				sessionDesc.preprocessorMacros = pInfo->SlangPreprocessorMacros;
				sessionDesc.preprocessorMacroCount = pInfo->SlangPreprocessorMacroCount;
			}

			std::array<slang::CompilerOptionEntry, 1> options =
			{
				{
					slang::CompilerOptionName::EmitSpirvDirectly,
					{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
				}
			};
			sessionDesc.compilerOptionEntries = options.data();
			sessionDesc.compilerOptionEntryCount = ( u32 )options.size();


			// TODO: Search paths in user projects
			if ( pInfo->SlangSessionSearchPaths && pInfo->SlangSessionSearchPathCount > 0 )
			{
				sessionDesc.searchPaths = pInfo->SlangSessionSearchPaths;
				sessionDesc.searchPathCount = pInfo->SlangSessionSearchPathCount;
			}
			
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

	void CShader::CompileShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;


		static auto writeEntrypointIntoMap = [&]( SlangStage stage, const char* name )
			{
				switch ( stage )
				{
				case SLANG_STAGE_VERTEX:
					m_Entrypoints[RHI::MW_SHADER_STAGE_VERTEX] = std::string( name );
					break;
				case SLANG_STAGE_HULL:
					m_Entrypoints[RHI::MW_SHADER_STAGE_TESSELATION_CONTROL] = std::string( name );
					break;
				case SLANG_STAGE_DOMAIN:
					m_Entrypoints[RHI::MW_SHADER_STAGE_TESSELATION_EVALUATION] = std::string( name );
					break;
				case SLANG_STAGE_FRAGMENT:
					m_Entrypoints[RHI::MW_SHADER_STAGE_FRAGMENT] = std::string( name );
					break;
				case SLANG_STAGE_GEOMETRY:
					m_Entrypoints[RHI::MW_SHADER_STAGE_GEOMETRY] = std::string( name );
					break;
				case SLANG_STAGE_COMPUTE:
					m_Entrypoints[RHI::MW_SHADER_STAGE_COMPUTE] = std::string( name );
					break;
				default:
					break;
				}
			};

		constexpr static auto stripSlangExtension = []( const std::string& path )
			{
				constexpr std::string_view suffix = ".slang";

				if ( path.ends_with( suffix ) )
					return path.substr( 0, path.size() - suffix.size() );

				return path;
			};

		Slang::ComPtr<slang::IModule> slangModule;

		{
			Slang::ComPtr<slang::IBlob> diagnostics;

			const std::string moduleName =
				stripSlangExtension( m_Path.string() );

			slangModule =
				m_SlangSession->loadModule(
					moduleName.c_str(),
					diagnostics.writeRef()
				);

			if ( diagnostics )
			{
				MW_ERROR(
					"Failed to compile or load Slang Module {}: {}",
					moduleName,
					static_cast< const char* >( diagnostics->getBufferPointer() )
				);
			}

			if ( !slangModule )
			{
				MW_ERROR(
					"Failed to load Slang Module {}",
					moduleName
				);

				return;
			}
		}


		const SlangInt entryPointCount =
			slangModule->getDefinedEntryPointCount();

		if ( entryPointCount == 0 )
		{
			MW_ERROR(
				"Slang Module {} contains no entry points",
				m_Path.string()
			);

			return;
		}

		std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints;
		entryPoints.reserve( entryPointCount );

		std::vector<slang::IComponentType*> components;
		components.reserve( entryPointCount + 1 );

		components.push_back( slangModule );

		for ( size_t i = 0; i < static_cast< size_t >( entryPointCount ); ++i )
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint;

			const SlangResult result =
				slangModule->getDefinedEntryPoint(
					static_cast<SlangInt32>( i ),
					entryPoint.writeRef()
				);

			if ( SLANG_FAILED( result ) || !entryPoint )
			{
				MW_ERROR(
					"Failed to get entry point {} from Slang Module {}",
					i,
					m_Path.string()
				);

				return;
			}

			entryPoints.push_back( entryPoint );
			components.push_back( entryPoint );
		}

		Slang::ComPtr<slang::IComponentType> composite;

		{
			Slang::ComPtr<slang::IBlob> diagnostics;

			const SlangResult result =
				m_SlangSession->createCompositeComponentType(
					components.data(),
					static_cast< SlangInt >( components.size() ),
					composite.writeRef(),
					diagnostics.writeRef()
				);

			if ( SLANG_FAILED( result ) )
			{
				if ( diagnostics )
				{
					MW_ERROR(
						"Failed to create Slang composite for {}: {}",
						m_Path.string(),
						static_cast< const char* >(
							diagnostics->getBufferPointer()
							)
					);
				}

				return;
			}
		}

		{
			Slang::ComPtr<slang::IBlob> diagnostics;

			const SlangResult result =
				composite->link(
					m_SlangProgram.writeRef(),
					diagnostics.writeRef()
				);

			if ( SLANG_FAILED( result ) )
			{
				if ( diagnostics )
				{
					MW_ERROR(
						"Failed to link Slang program {}: {}",
						m_Path.string(),
						static_cast< const char* >(
							diagnostics->getBufferPointer()
							)
					);
				}

				return;
			}
		}

		Slang::ComPtr<slang::IBlob> code;
		Slang::ComPtr<slang::IBlob> diagnostics;

		const SlangResult result =
			m_SlangProgram->getTargetCode(
				0,
				code.writeRef(),
				diagnostics.writeRef()
			);

		if ( SLANG_FAILED( result ) )
		{
			if ( diagnostics )
			{
				MW_ERROR(
					"Failed to generate target code for {}: {}",
					m_Path.string(),
					static_cast< const char* >(
						diagnostics->getBufferPointer()
						)
				);
			}

			return;
		}

		MW_INFO(
			"Successfully compiled and linked Slang shader {}",
			m_Path.string()
		);
		
		slang::ProgramLayout* layout = m_SlangProgram->getLayout();
		for ( int i = 0; i < layout->getEntryPointCount(); ++i ) {
			slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );
			SlangStage stage = entryPointLayout->getStage(); 
			writeEntrypointIntoMap( stage, entryPointLayout->getName() );
		}
	}

	void CShader::ReflectOnShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;

	}


}

