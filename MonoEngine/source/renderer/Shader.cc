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
	using namespace RHI;
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

		for ( auto i{ 0uz }; i < static_cast< size_t >( entryPointCount ); ++i )
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

	ShaderReflectionData CShader::ReflectOnShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		// TODO: Garantee that global data is in set 0.

		ShaderReflectionData reflectionData{};

#ifdef MW_VULKAN

		if ( !m_pSlangProgram )
		{
			MW_ERROR(
				"Cannot reflect shader {}: Slang program is null",
				m_Path.string()
			);

			return reflectionData;
		}

		slang::ProgramLayout* pLayout =
			m_pSlangProgram->getLayout( 0 );

		if ( !pLayout )
		{
			MW_ERROR(
				"Cannot reflect shader {}: ProgramLayout is null",
				m_Path.string()
			);

			return reflectionData;
		}


		const VkDevice device = *RHI::CVulkanContext::GetDevice()->GetDevice();

		if ( device == VK_NULL_HANDLE )
		{
			MW_ERROR( "Cannot reflect shader {}: VkDevice is null", m_Path.string() );
			return reflectionData;
		}


		auto getDescriptorType = []( slang::BindingType type ) -> VkDescriptorType
			{
				switch ( type )
				{
				case slang::BindingType::Sampler:
					return VK_DESCRIPTOR_TYPE_SAMPLER;

				case slang::BindingType::Texture:
					return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

				case slang::BindingType::MutableTexture:
					return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

				case slang::BindingType::ConstantBuffer:
					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				case slang::BindingType::RawBuffer:
				case slang::BindingType::MutableRawBuffer:
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

				case slang::BindingType::TypedBuffer:
					return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

				case slang::BindingType::MutableTypedBuffer:
					return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

				case slang::BindingType::CombinedTextureSampler:
					return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

				default:
					return VK_DESCRIPTOR_TYPE_MAX_ENUM;
				}
			};

		auto getVkShaderStage = []( SlangStage stage ) -> VkShaderStageFlags
			{
				switch ( stage )
				{
				case SLANG_STAGE_VERTEX:
					return VK_SHADER_STAGE_VERTEX_BIT;

				case SLANG_STAGE_HULL:
					return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

				case SLANG_STAGE_DOMAIN:
					return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

				case SLANG_STAGE_GEOMETRY:
					return VK_SHADER_STAGE_GEOMETRY_BIT;

				case SLANG_STAGE_FRAGMENT:
					return VK_SHADER_STAGE_FRAGMENT_BIT;

				case SLANG_STAGE_COMPUTE:
					return VK_SHADER_STAGE_COMPUTE_BIT;

				default:
					return 0;
				}
			};



		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> descriptorSetBindings;
		std::vector<VkPushConstantRange> pushConstantRanges;

		auto addDescriptor = [&]( uint32_t set, uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags )
			{
				if ( descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
					return;

				auto& bindings = descriptorSetBindings[set];


				for ( VkDescriptorSetLayoutBinding& existing : bindings )
				{
					if ( existing.binding == binding && existing.descriptorType == descriptorType )
					{
						existing.stageFlags |= stageFlags;

						if ( existing.descriptorCount != descriptorCount && descriptorCount != VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT )
						{
							existing.descriptorCount = ( ( ( existing.descriptorCount ) > ( descriptorCount ) ) ? ( existing.descriptorCount ) : ( descriptorCount ) );
						}

						return;
					}
				}


				VkDescriptorSetLayoutBinding layoutBinding{};
				layoutBinding.binding = binding;
				layoutBinding.descriptorType = descriptorType;
				layoutBinding.descriptorCount = descriptorCount;
				layoutBinding.stageFlags = stageFlags;

				bindings.push_back( layoutBinding );
			};

		auto addPushConstant = [&]( uint32_t offset, uint32_t size, VkShaderStageFlags stageFlags )
			{
				if ( size == 0 || stageFlags == 0 )
					return;


				for ( VkPushConstantRange& existing : pushConstantRanges )
				{
					if ( existing.offset == offset && existing.size == size )
					{
						existing.stageFlags |= stageFlags;
						return;
					}
				}

				VkPushConstantRange range{};
				range.offset = offset;
				range.size = size;
				range.stageFlags = stageFlags;

				pushConstantRanges.push_back( range );
			};


		auto reflectType = [&]( slang::TypeLayoutReflection* pTypeLayout, VkShaderStageFlags stageFlags )
			{
				if ( !pTypeLayout )
					return;

				const SlangInt setCount = pTypeLayout->getDescriptorSetCount();


				for ( auto relativeSet{ 0uz }; relativeSet < setCount; ++relativeSet ) // NOTE: Iterates over all sets
				{
					const SlangInt set = pTypeLayout->getDescriptorSetSpaceOffset( relativeSet ); // NOTE: this is the number of the actual set ( layout ( set = x )).

					if ( set < 0 )
						continue;

					const SlangInt rangeCount = pTypeLayout->getDescriptorSetDescriptorRangeCount( relativeSet );

					for ( auto rangeIndex{ 0uz }; rangeIndex < rangeCount; ++rangeIndex ) // these are the seperate bindings / VkDescriptorSetLayoutBinding 
					{
						const slang::BindingType bindingType = pTypeLayout->getDescriptorSetDescriptorRangeType( relativeSet, rangeIndex );
						if ( bindingType == slang::BindingType::PushConstant )
							continue;

						const VkDescriptorType descriptorType = getDescriptorType( bindingType );
						if ( descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
							continue;

						const SlangInt binding = pTypeLayout->getDescriptorSetDescriptorRangeIndexOffset( relativeSet, rangeIndex );
						if ( binding == SLANG_UNKNOWN_SIZE )
						{
							MW_ERROR( "Shader {} contains a descriptor binding whose Vulkan binding is unresolved", m_Path.string() );
							continue;
						}


						const SlangInt descriptorCount = pTypeLayout->getDescriptorSetDescriptorRangeDescriptorCount( relativeSet, rangeIndex );


						uint32_t vkDescriptorCount = 1;

						if ( descriptorCount == SLANG_UNBOUNDED_SIZE )
						{
							vkDescriptorCount = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
						}
						else
						{
							if ( descriptorCount <= 0 )
							{
								vkDescriptorCount = 1;
							}
							else
							{
								vkDescriptorCount = static_cast< uint32_t >( descriptorCount );
							}
						}


						addDescriptor(
							static_cast< uint32_t >( set ),
							static_cast< uint32_t >( binding ),
							vkDescriptorCount,
							descriptorType,
							stageFlags
						);
					}
				}
			};
	

		{
			slang::VariableLayoutReflection* pGlobalParams = pLayout->getGlobalParamsVarLayout();

			if ( pGlobalParams )
			{
				reflectType( pGlobalParams->getTypeLayout(), VK_SHADER_STAGE_ALL );
			}
		}

		const auto entryPointCount = pLayout->getEntryPointCount();

		for ( auto entryPointIndex{ 0uz }; entryPointIndex < entryPointCount; ++entryPointIndex )
		{
			slang::EntryPointLayout* pEntryPoint = pLayout->getEntryPointByIndex( entryPointIndex );

			if ( !pEntryPoint )
				continue;

			const SlangStage slangStage = pEntryPoint->getStage();

			const VkShaderStageFlags stageFlags = getVkShaderStage( slangStage );

			if ( !stageFlags )
				continue;

			slang::TypeLayoutReflection* pTypeLayout = pEntryPoint->getTypeLayout();

			if ( pTypeLayout )
				reflectType( pTypeLayout, stageFlags );
			
		}

		auto reflectPushConstants = [&]( slang::TypeLayoutReflection* pTypeLayout, VkShaderStageFlags stageFlags )
			{
				if ( !pTypeLayout )
					return;


				const SlangInt bindingRangeCount = pTypeLayout->getBindingRangeCount();

				for ( auto rangeIndex{ 0uz }; rangeIndex < bindingRangeCount; ++rangeIndex )
				{
					const slang::BindingType bindingType = pTypeLayout->getBindingRangeType( rangeIndex );

					if ( bindingType != slang::BindingType::PushConstant )
						continue;

					slang::TypeLayoutReflection* pLeafType = pTypeLayout->getBindingRangeLeafTypeLayout( rangeIndex );

					if ( !pLeafType )
						continue;

					const SlangInt size = pLeafType->getSize( slang::ParameterCategory::PushConstantBuffer );

					if ( size == SLANG_UNKNOWN_SIZE || size <= 0 )
						continue;

					addPushConstant(
						0,
						static_cast< uint32_t >( size ),
						stageFlags
					);
				}
			};

		{
			slang::VariableLayoutReflection* pGlobalParams = pLayout->getGlobalParamsVarLayout();

			if ( pGlobalParams )
				reflectPushConstants( pGlobalParams->getTypeLayout(), VK_SHADER_STAGE_ALL );
		}

		for ( auto entryPointIndex{ 0uz }; entryPointIndex < entryPointCount; ++entryPointIndex )
		{
			slang::EntryPointLayout* pEntryPoint = pLayout->getEntryPointByIndex( entryPointIndex );

			if ( !pEntryPoint )
				continue;


			const VkShaderStageFlags stageFlags = getVkShaderStage( pEntryPoint->getStage() );

			if ( !stageFlags )
				continue;


			reflectPushConstants( pEntryPoint->getTypeLayout(), stageFlags );
		}

		for ( auto& [set, bindings] : descriptorSetBindings )
		{
			std::sort( bindings.begin(), bindings.end(),
				[]( const VkDescriptorSetLayoutBinding& lhs,
					const VkDescriptorSetLayoutBinding& rhs )
				{
					return lhs.binding < rhs.binding;
				}
			);
		}


		std::sort( pushConstantRanges.begin(), pushConstantRanges.end(),
			[]( const VkPushConstantRange& lhs,
				const VkPushConstantRange& rhs )
			{
				if ( lhs.offset != rhs.offset )
					return lhs.offset < rhs.offset;

				return lhs.size < rhs.size;
			}
		);


		u32 highestSet = 0;

		if ( !descriptorSetBindings.empty() )
			highestSet = descriptorSetBindings.rbegin()->first;
		
		std::vector<VkDescriptorSetLayout> vkSetLayouts;
		vkSetLayouts.resize(
			static_cast< size_t >( highestSet ) + 1,
			nullptr
		);


		for ( auto& [set, bindings] : descriptorSetBindings )
		{
			VkDescriptorSetLayoutCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.bindingCount = static_cast< u32 >( bindings.size() );
			createInfo.pBindings = bindings.data();

			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
			bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;

			std::vector<VkDescriptorBindingFlags> bindingFlags;

			bool hasVariableBinding = false;

			for ( const VkDescriptorSetLayoutBinding& binding : bindings )
			{
				if ( binding.descriptorCount == VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT )
				{
					hasVariableBinding = true;
					break;
				}
			}


			if ( hasVariableBinding )
			{
				bindingFlags.resize( bindings.size(), 0 );


				for ( auto i{ 0uz }; i < bindings.size(); ++i )
				{
					if ( bindings[i].descriptorCount == VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT )
						bindingFlags[i] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
				}


				bindingFlagsInfo.bindingCount = static_cast< u32 >( bindingFlags.size() );
				bindingFlagsInfo.pBindingFlags = bindingFlags.data();

				createInfo.pNext = &bindingFlagsInfo;
			}

			for ( VkDescriptorSetLayoutBinding& binding : bindings )
			{
				if ( binding.descriptorCount == VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT )
					binding.descriptorCount = 1;
			}

			VkDescriptorSetLayout layoutHandle = nullptr;

			const VkResult result =
				vkCreateDescriptorSetLayout(
					device,
					&createInfo,
					CVulkanContext::GetCallbacks(),
					&layoutHandle
				);


			if ( result != VK_SUCCESS )
			{
				MW_ERROR( "Failed to create VkDescriptorSetLayout for shader {} set {}. VkResult = {}", m_Path.string(), set, static_cast<int>( result ) );

				for ( VkDescriptorSetLayout handle : vkSetLayouts )
				{
					if ( handle != VK_NULL_HANDLE )
					{
						vkDestroyDescriptorSetLayout(
							device,
							handle,
							CVulkanContext::GetCallbacks()
						);
					}
				}

				return reflectionData;
			}


			vkSetLayouts[set] =
				layoutHandle;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

		pipelineLayoutInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount =
			static_cast< uint32_t >(
				vkSetLayouts.size()
				);

		pipelineLayoutInfo.pSetLayouts =
			vkSetLayouts.empty()
			? nullptr
			: vkSetLayouts.data();

		pipelineLayoutInfo.pushConstantRangeCount =
			static_cast< uint32_t >(
				pushConstantRanges.size()
				);

		pipelineLayoutInfo.pPushConstantRanges =
			pushConstantRanges.empty()
			? nullptr
			: pushConstantRanges.data();


		VkPipelineLayout pipelineLayout =
			VK_NULL_HANDLE;


		const VkResult pipelineResult =
			vkCreatePipelineLayout(
				device,
				&pipelineLayoutInfo,
				CVulkanContext::GetCallbacks(),
				&pipelineLayout
			);


		if ( pipelineResult != VK_SUCCESS )
		{
			MW_ERROR(
				"Failed to create VkPipelineLayout for shader {}. "
				"VkResult = {}",
				m_Path.string(),
				static_cast< int >( pipelineResult )
			);


			for ( VkDescriptorSetLayout handle :
			vkSetLayouts )
			{
				if ( handle != VK_NULL_HANDLE )
				{
					vkDestroyDescriptorSetLayout(
						device,
						handle,
						CVulkanContext::GetCallbacks()
					);
				}
			}

			return reflectionData;
		}


		reflectionData.pPipelineSignature = reinterpret_cast< RHI::PipelineSignature >(	pipelineLayout );


		reflectionData.pDescriptorSignatures.reserve( vkSetLayouts.size() );


		for ( VkDescriptorSetLayout layout : vkSetLayouts )
			reflectionData.pDescriptorSignatures.push_back( reinterpret_cast< RHI::DescriptorSignature >( layout ) );


		reflectionData.PushConstantRanges.reserve( pushConstantRanges.size() );

		for ( const VkPushConstantRange& range : pushConstantRanges )
		{
			PushConstantRanges pushConstant{};
			pushConstant.Offset = range.offset;
			pushConstant.Size = range.size;
			reflectionData.PushConstantRanges.push_back( pushConstant );
		}

		return reflectionData;

#else

		MW_ERROR( "ReflectOnShader() called without Vulkan support for {}", m_Path.string() );

		return reflectionData;

#endif
	}


}

