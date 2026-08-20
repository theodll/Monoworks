#include <mwpch.hh>

#include "Shader.hh"

#ifdef MW_VULKAN 
#include <rhi/specific/vulkan/VulkanContext.hh>
#endif

#include <core/Application.hh>
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
			
			globalSession->createSession( sessionDesc, m_pSlangSession.writeRef() );
		} 
		else 
		{
			if ( pInfo->SlangSession )
				m_pSlangSession = pInfo->SlangSession;
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
				m_pSlangSession->loadModule(
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
				m_pSlangSession->createCompositeComponentType(
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
					m_pSlangProgram.writeRef(),
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
			m_pSlangProgram->getTargetCode(
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
		
		slang::ProgramLayout* layout = m_pSlangProgram->getLayout();
		for ( int i = 0; i < layout->getEntryPointCount(); ++i ) {
			slang::EntryPointLayout* entryPointLayout = layout->getEntryPointByIndex( i );
			SlangStage stage = entryPointLayout->getStage(); 
			writeEntrypointIntoMap( stage, entryPointLayout->getName() );
		}
	}

	RHI::PipelineSignature CShader::ReflectOnShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		RHI::PipelineSignature signature = nullptr;
#ifdef MW_VULKAN
		auto getVkSig = [&]() -> VkPipelineLayout
			{

				slang::ProgramLayout* pLayout = m_pSlangProgram->getLayout();
				if ( !pLayout )
					return VK_NULL_HANDLE;

				auto mapDescriptorType = []( slang::BindingType T ) -> VkDescriptorType
					{
						switch ( T )
						{
						case slang::BindingType::Sampler:                         return VK_DESCRIPTOR_TYPE_SAMPLER;
						case slang::BindingType::CombinedTextureSampler:          return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						case slang::BindingType::Texture:                         return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
						case slang::BindingType::MutableTexture:                  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						case slang::BindingType::TypedBuffer:                     return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
						case slang::BindingType::MutableTypedBuffer:              return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
						case slang::BindingType::RawBuffer:                       return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						case slang::BindingType::MutableRawBuffer:                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						case slang::BindingType::ConstantBuffer:                  return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						case slang::BindingType::InlineUniformData:               return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
						case slang::BindingType::RayTracingAccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
						default: assert( false && "unhandled slang::BindingType" ); return VK_DESCRIPTOR_TYPE_MAX_ENUM;
						}
					};

				auto mapStage = []( SlangStage S ) -> VkShaderStageFlags
					{
						switch ( S )
						{
						case SLANG_STAGE_VERTEX:         return VK_SHADER_STAGE_VERTEX_BIT;
						case SLANG_STAGE_HULL:           return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
						case SLANG_STAGE_DOMAIN:         return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
						case SLANG_STAGE_GEOMETRY:       return VK_SHADER_STAGE_GEOMETRY_BIT;
						case SLANG_STAGE_FRAGMENT:       return VK_SHADER_STAGE_FRAGMENT_BIT;
						case SLANG_STAGE_COMPUTE:        return VK_SHADER_STAGE_COMPUTE_BIT;
						case SLANG_STAGE_RAY_GENERATION: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
						case SLANG_STAGE_INTERSECTION:   return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
						case SLANG_STAGE_ANY_HIT:        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
						case SLANG_STAGE_CLOSEST_HIT:    return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
						case SLANG_STAGE_MISS:           return VK_SHADER_STAGE_MISS_BIT_KHR;
						case SLANG_STAGE_CALLABLE:       return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
						case SLANG_STAGE_MESH:           return VK_SHADER_STAGE_MESH_BIT_EXT;
						case SLANG_STAGE_AMPLIFICATION:  return VK_SHADER_STAGE_TASK_BIT_EXT;
						default: assert( false && "unhandled SlangStage" ); return VkShaderStageFlags( 0 );
						}
					};

				std::vector<VkDescriptorSetLayout> setLayouts;
				std::vector<VkPushConstantRange>   pushConstantRanges;

				std::function<void( std::vector<VkDescriptorSetLayoutBinding>& bindings, slang::TypeLayoutReflection* pElem, VkShaderStageFlags stage )> addElement =
					[&]( std::vector<VkDescriptorSetLayoutBinding>& bindings, slang::TypeLayoutReflection* pElem, VkShaderStageFlags stage )
					{
						if ( pElem->getSize() > 0 )
							bindings.push_back( { ( u32 )bindings.size(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, stage, nullptr } );

						for ( auto i{ 0uz }; i < pElem->getDescriptorSetDescriptorRangeCount( 0 ); ++i )
						{
							const slang::BindingType bt = pElem->getDescriptorSetDescriptorRangeType( 0, i );
							if ( bt == slang::BindingType::PushConstant )
								continue;

							const u32 count = ( u32 )pElem->getDescriptorSetDescriptorRangeDescriptorCount( 0, i );
							bindings.push_back( { ( u32 )bindings.size(), mapDescriptorType( bt ), count, stage, nullptr } );
						}

						for ( auto i{ 0uz }; i < pElem->getSubObjectRangeCount(); ++i )
						{
							const int bindingRangeIndex = pElem->getSubObjectRangeBindingRangeIndex( i );
							const slang::BindingType bt = pElem->getBindingRangeType( bindingRangeIndex );

							if ( bt == slang::BindingType::ParameterBlock )
							{
								slang::TypeLayoutReflection* pNested = pElem->getBindingRangeLeafTypeLayout( bindingRangeIndex );

								const size_t setIndex = setLayouts.size();
								setLayouts.push_back( VK_NULL_HANDLE );

								std::vector<VkDescriptorSetLayoutBinding> nestedBindings;
								addElement( nestedBindings, pNested->getElementTypeLayout(), stage );

								if ( !nestedBindings.empty() )
								{
									VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
									info.bindingCount = ( u32 )nestedBindings.size();
									info.pBindings = nestedBindings.data();
									vkCreateDescriptorSetLayout( *RHI::CVulkanContext::GetDevice()->GetDevice(), &info, RHI::CVulkanContext::GetCallbacks(), &setLayouts[setIndex] );
								}
							}
							else if ( bt == slang::BindingType::PushConstant )
							{
								slang::TypeLayoutReflection* pCB = pElem->getBindingRangeLeafTypeLayout( bindingRangeIndex )->getElementTypeLayout();
								if ( const size_t Size = pCB->getSize() )
									pushConstantRanges.push_back( { stage, 0, ( u32 )Size } );
							}
						}
					};


				setLayouts.push_back( nullptr );

				std::vector<VkDescriptorSetLayoutBinding> defaultBindings;
				addElement( defaultBindings, pLayout->getGlobalParamsTypeLayout(), VK_SHADER_STAGE_ALL );

				for ( auto i{ 0uz }; i < ( size_t )pLayout->getEntryPointCount(); ++i )
				{
					slang::EntryPointLayout* pEntry = pLayout->getEntryPointByIndex( i );
					addElement( defaultBindings, pEntry->getTypeLayout(), mapStage( pEntry->getStage() ) );
				}

				if ( !defaultBindings.empty() )
				{
					VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
					info.bindingCount = ( u32 )defaultBindings.size();
					info.pBindings = defaultBindings.data();
					vkCreateDescriptorSetLayout( *RHI::CVulkanContext::GetDevice()->GetDevice(), &info, RHI::CVulkanContext::GetCallbacks(), &setLayouts[0] );
				}

				VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
				layoutInfo.setLayoutCount = ( u32 )setLayouts.size();
				layoutInfo.pSetLayouts = setLayouts.data();
				layoutInfo.pushConstantRangeCount = ( u32 )pushConstantRanges.size();
				layoutInfo.pPushConstantRanges = pushConstantRanges.data();

				VkPipelineLayout pipelineLayout = nullptr;
				vkCreatePipelineLayout( *RHI::CVulkanContext::GetDevice()->GetDevice(), &layoutInfo, RHI::CVulkanContext::GetCallbacks(), &pipelineLayout );
				return pipelineLayout;
		};	

	#endif

		
		switch ( CApplication::GetGraphicsAPI() )
		{
		case MW_GAPI_NONE:    return nullptr;
#ifdef MW_VULKAN
		case MW_GAPI_VULKAN:  return static_cast< RHI::PipelineSignature >( getVkSig() );
#endif
		default:
			MW_ERROR( "Invalid Graphics API. ");
			return nullptr;
		}
		
	}


}

