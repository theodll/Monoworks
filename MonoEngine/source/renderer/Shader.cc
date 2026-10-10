#include <mwpch.hh>

#include "Shader.hh"

#ifdef MW_VULKAN 
#include <rhi/specific/vulkan/VulkanContext.hh>
#endif

#include <core/Application.hh>
#include <renderer/StaticRenderer.hh>

#include <slang.h>
#include <slang-com-ptr.h>

#include <algorithm>
#include <functional>
#include <set>

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
			targetDesc.profile = globalSession->findProfile( "spirv_2" );

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
				MW_WARN(
					"Might have failed to compile or load Slang Module {}: {}",
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
					static_cast< SlangInt32 >( i ),
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

	bool CShader::HasGlobalBindings() const NOEXCEPT
	{
#ifdef MW_VULKAN

		if ( !m_pSlangProgram )
			return false;

		slang::ProgramLayout* pLayout =
			m_pSlangProgram->getLayout( 0 );

		if ( !pLayout )
			return false;

		slang::VariableLayoutReflection* pGlobalParams =
			pLayout->getGlobalParamsVarLayout();

		if ( !pGlobalParams )
			return false;

		slang::TypeLayoutReflection* pTypeLayout =
			pGlobalParams->getTypeLayout();

		if ( !pTypeLayout )
			return false;

		const SlangInt bindingRangeCount =
			pTypeLayout->getBindingRangeCount();

		for (
			SlangInt i = 0;
			i < bindingRangeCount;
			++i
			)
		{
			const slang::BindingType type =
				pTypeLayout->getBindingRangeType( i );

			if (
				type != slang::BindingType::PushConstant &&
				type != slang::BindingType::ParameterBlock
				)
			{
				return true;
			}
		}

		return false;

#else

		return false;

#endif
	}

	ShaderReflectionData CShader::ReflectOnShader() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( m_bReflected )
			return m_ReflectionData;

		m_bReflected = true;

		ShaderReflectionData reflectionData{};

#ifdef MW_VULKAN

		/*
		 * -------------------------------------------------------------------------
		 * Overview
		 * -------------------------------------------------------------------------
		 *
		 * There are two fundamentally different kinds of descriptor information
		 * that Slang exposes through reflection:
		 *
		 *  1. Descriptor ranges that belong directly to a type layout.
		 *
		 *     These are queried through:
		 *
		 *         getDescriptorSetCount()
		 *         getDescriptorSetSpaceOffset()
		 *         getDescriptorSetDescriptorRangeCount()
		 *         ...
		 *
		 *  2. Sub-object ranges.
		 *
		 *     A ParameterBlock<> is represented as a sub-object of its parent
		 *     layout. It therefore does NOT contribute its descriptor sets to the
		 *     parent's getDescriptorSetCount().
		 *
		 *     ParameterBlock<>s must instead be discovered through:
		 *
		 *         getSubObjectRangeCount()
		 *         getSubObjectRangeBindingRangeIndex()
		 *         getSubObjectRangeSpaceOffset()
		 *
		 *     and their contents are reflected from the ParameterBlock's element
		 *     type layout.
		 *
		 * This distinction is extremely important. In particular, simply calling
		 * reflectType() recursively on the global or entry-point type layout would
		 * miss ParameterBlock<> contents completely.
		 *
		 * The resulting descriptor-set vector is deliberately indexed by the
		 * actual Vulkan set number:
		 *
		 *     pDescriptorSignatures[0] -> Vulkan set 0
		 *     pDescriptorSignatures[1] -> Vulkan set 1
		 *     ...
		 *
		 * Every index up to the highest referenced set is materialized. This means
		 * that holes are represented by empty VkDescriptorSetLayouts rather than
		 * by null handles.
		 *
		 * Set 0 is always reserved in the vector, even if it contains zero
		 * bindings.
		 *
		 * IMPORTANT:
		 *
		 * We do NOT invent new Vulkan set numbers for ParameterBlocks. The set
		 * number reported by Slang is part of the compiled shader interface and
		 * therefore must be preserved. In the common automatic-binding case,
		 * ParameterBlocks are allocated by Slang in increasing set order.
		 *
		 * If a shader contains only ParameterBlocks and no ordinary global
		 * parameters, Slang is allowed to put the first ParameterBlock into set 0.
		 * In that case set 0 is necessarily the ParameterBlock set; creating a
		 * separate empty "global set 0" and moving the block to set 1 would make
		 * the Vulkan pipeline layout disagree with the generated SPIR-V.
		 */

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


		const VkDevice device =
			*RHI::CVulkanContext::GetDevice()->GetDevice();

		if ( device == VK_NULL_HANDLE )
		{
			MW_ERROR(
				"Cannot reflect shader {}: VkDevice is null",
				m_Path.string()
			);

			return reflectionData;
		}


		MW_TRACE(
			"Beginning Slang reflection for shader {}",
			m_Path.string()
		);


		/*
		 * -------------------------------------------------------------------------
		 * Slang -> Vulkan descriptor type mapping
		 * -------------------------------------------------------------------------
		 *
		 * A BindingType describes the kind of resource represented by a reflected
		 * binding range. ParameterBlock itself is intentionally not mapped here:
		 * it is a sub-object and gets handled separately below.
		 */
		auto getDescriptorType =
			[]( slang::BindingType type ) -> VkDescriptorType
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

				case slang::BindingType::InputRenderTarget:
					return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

				case slang::BindingType::InlineUniformData:
					return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;

				case slang::BindingType::RayTracingAccelerationStructure:
					return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

					/*
					 * These are not Vulkan descriptor bindings:
					 *
					 *  PushConstant
					 *  ParameterBlock
					 *  varying inputs/outputs
					 *  existential values
					 *  mutable flags
					 *  type-system masks
					 */
				default:
					return VK_DESCRIPTOR_TYPE_MAX_ENUM;
				}
			};


		auto getVkShaderStage =
			[]( SlangStage stage ) -> VkShaderStageFlags
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


		auto getStageName =
			[]( SlangStage stage ) -> const char*
			{
				switch ( stage )
				{
				case SLANG_STAGE_VERTEX:
					return "vertex";

				case SLANG_STAGE_HULL:
					return "tessellation-control";

				case SLANG_STAGE_DOMAIN:
					return "tessellation-evaluation";

				case SLANG_STAGE_GEOMETRY:
					return "geometry";

				case SLANG_STAGE_FRAGMENT:
					return "fragment";

				case SLANG_STAGE_COMPUTE:
					return "compute";

				default:
					return "unknown";
				}
			};


		/*
		 * -------------------------------------------------------------------------
		 * Intermediate descriptor representation
		 * -------------------------------------------------------------------------
		 *
		 * We keep VkDescriptorBindingFlags next to the Vulkan binding itself.
		 * This is particularly useful for unbounded Slang arrays.
		 *
		 * descriptorCount for VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
		 * is intentionally initialized to 1 here. Vulkan still requires a concrete
		 * maximum descriptor count in VkDescriptorSetLayoutBinding; the actual
		 * runtime descriptor count can be selected when the descriptor set is
		 * allocated.
		 */
		struct DescriptorBinding
		{
			VkDescriptorSetLayoutBinding Binding{};
			VkDescriptorBindingFlags Flags = 0;
		};


		/*
		 * Index 0 is created immediately.
		 *
		 * This is the invariant requested by the renderer:
		 *
		 *     pDescriptorSignatures[0] always exists.
		 *
		 * If there are no actual bindings belonging to set 0, the Vulkan descriptor
		 * set layout created for it will simply have bindingCount = 0.
		 */
		std::vector<std::vector<DescriptorBinding>> descriptorSetBindings;
		descriptorSetBindings.resize( 1 );


		auto ensureSet =
			[&]( SlangInt set ) -> bool
			{
				if ( set < 0 )
				{
					MW_ERROR(
						"Shader {} contains a descriptor set with an invalid "
						"negative set index {}",
						m_Path.string(),
						set
					);

					return false;
				}


				const size_t setIndex =
					static_cast< size_t >( set );


				if ( setIndex >= descriptorSetBindings.size() )
				{
					MW_TRACE(
						"Growing reflected descriptor-set array for shader {} "
						"from {} to {} entries",
						m_Path.string(),
						descriptorSetBindings.size(),
						setIndex + 1
					);

					descriptorSetBindings.resize( setIndex + 1 );
				}


				return true;
			};


		/*
		 * Add one descriptor to a reflected Vulkan set.
		 *
		 * Two shader stages may legally contribute the same binding to the same
		 * descriptor set. In that case their stage flags are merged.
		 *
		 * A binding number with two different descriptor types is not legal in a
		 * Vulkan descriptor set layout, so such a collision is reported.
		 */
		auto addDescriptor =
			[&](
				uint32_t set,
				uint32_t binding,
				uint32_t descriptorCount,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				VkDescriptorBindingFlags bindingFlags,
				const char* debugName
				)
			{
				if ( descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
				{
					MW_TRACE(
						"Skipping unsupported descriptor '{}' in shader {} "
						"at set {} binding {}",
						debugName ? debugName : "<unnamed>",
						m_Path.string(),
						set,
						binding
					);

					return;
				}


				if ( descriptorCount == 0 )
				{
					MW_ERROR(
						"Shader {} produced descriptor '{}' at set {} binding {} "
						"with descriptorCount = 0",
						m_Path.string(),
						debugName ? debugName : "<unnamed>",
						set,
						binding
					);

					return;
				}


				if ( !ensureSet( static_cast< SlangInt >( set ) ) )
					return;


				auto& bindings =
					descriptorSetBindings[set];


				for ( DescriptorBinding& existing : bindings )
				{
					if ( existing.Binding.binding != binding )
						continue;


					if ( existing.Binding.descriptorType != descriptorType )
					{
						MW_ERROR(
							"Shader {} has incompatible descriptor bindings at "
							"set {} binding {}: '{}' uses descriptor type {}, "
							"while an existing range uses descriptor type {}",
							m_Path.string(),
							set,
							binding,
							debugName ? debugName : "<unnamed>",
							static_cast< int >( descriptorType ),
							static_cast< int >( existing.Binding.descriptorType )
						);

						return;
					}


					existing.Binding.stageFlags |= stageFlags;
					existing.Flags |= bindingFlags;

					if ( descriptorCount > existing.Binding.descriptorCount )
						existing.Binding.descriptorCount = descriptorCount;


					MW_TRACE(
						"Merged descriptor '{}' into shader {} set {} binding {} "
						"(descriptorCount = {}, stageFlags = 0x{:X})",
						debugName ? debugName : "<unnamed>",
						m_Path.string(),
						set,
						binding,
						existing.Binding.descriptorCount,
						existing.Binding.stageFlags
					);

					return;
				}


				DescriptorBinding descriptorBinding{};

				descriptorBinding.Binding.binding =
					binding;

				descriptorBinding.Binding.descriptorType =
					descriptorType;

				descriptorBinding.Binding.descriptorCount =
					descriptorCount;

				descriptorBinding.Binding.stageFlags =
					stageFlags;

				descriptorBinding.Flags =
					bindingFlags;


				bindings.push_back( descriptorBinding );


				MW_TRACE(
					"Added descriptor '{}' to shader {} set {} binding {} "
					"(type = {}, descriptorCount = {}, stageFlags = 0x{:X}, "
					"bindingFlags = 0x{:X})",
					debugName ? debugName : "<unnamed>",
					m_Path.string(),
					set,
					binding,
					static_cast< int >( descriptorType ),
					descriptorCount,
					stageFlags,
					bindingFlags
				);
			};


		/*
		 * -------------------------------------------------------------------------
		 * Push constants
		 * -------------------------------------------------------------------------
		 */
		std::vector<VkPushConstantRange> pushConstantRanges;


		auto addPushConstant =
			[&](
				uint32_t offset,
				uint32_t size,
				VkShaderStageFlags stageFlags
				)
			{
				if ( size == 0 || stageFlags == 0 )
					return;


				for ( VkPushConstantRange& existing : pushConstantRanges )
				{
					if (
						existing.offset == offset &&
						existing.size == size
						)
					{
						existing.stageFlags |= stageFlags;

						MW_TRACE(
							"Merged push constant range in shader {} "
							"(offset = {}, size = {}, stageFlags = 0x{:X})",
							m_Path.string(),
							offset,
							size,
							existing.stageFlags
						);

						return;
					}
				}


				VkPushConstantRange range{};

				range.offset =
					offset;

				range.size =
					size;

				range.stageFlags =
					stageFlags;


				pushConstantRanges.push_back( range );


				MW_TRACE(
					"Added push constant range to shader {} "
					"(offset = {}, size = {}, stageFlags = 0x{:X})",
					m_Path.string(),
					offset,
					size,
					stageFlags
				);
			};


		/*
		 * -------------------------------------------------------------------------
		 * Direct type-layout reflection
		 * -------------------------------------------------------------------------
		 *
		 * THIS FUNCTION INTENTIONALLY DOES NOT WALK SUB-OBJECT RANGES.
		 *
		 * This is the exact distinction that matters for ParameterBlock<>:
		 *
		 *     getDescriptorSetCount()
		 *
		 * walks the descriptor sets belonging directly to this type layout.
		 *
		 * A ParameterBlock<> nested below this layout is not one of those sets.
		 * It is handled separately by reflectParameterBlocks() below.
		 */
		auto reflectType =
			[&](
				slang::TypeLayoutReflection* pTypeLayout,
				VkShaderStageFlags stageFlags
				)
			{

				// STAGE FLAGS IS NOT BEING SET! 
				// TODO: set shader stage !
				if ( !pTypeLayout || stageFlags == 0 )
					return;


				const SlangInt setCount =
					pTypeLayout->getDescriptorSetCount();


				MW_TRACE(
					"Reflecting {} direct descriptor sets in shader {} "
					"(stageFlags = 0x{:X})",
					setCount,
					m_Path.string(),
					stageFlags
				);


				for (
					SlangInt relativeSet = 0;
					relativeSet < setCount;
					++relativeSet
					)
				{
					const SlangInt set =
						pTypeLayout->getDescriptorSetSpaceOffset(
							relativeSet
						);


					if ( set < 0 )
					{
						MW_ERROR(
							"Shader {} returned an invalid descriptor-set "
							"space offset {} for relative set {}",
							m_Path.string(),
							set,
							relativeSet
						);

						continue;
					}


					if ( !ensureSet( set ) )
						continue;


					const SlangInt rangeCount =
						pTypeLayout->getDescriptorSetDescriptorRangeCount(
							relativeSet
						);


					MW_TRACE(
						"Shader {} direct set {} contains {} descriptor ranges",
						m_Path.string(),
						set,
						rangeCount
					);


					for (
						SlangInt rangeIndex = 0;
						rangeIndex < rangeCount;
						++rangeIndex
						)
					{
						const slang::BindingType bindingType =
							pTypeLayout->getDescriptorSetDescriptorRangeType(
								relativeSet,
								rangeIndex
							);


						/*
						 * Push constants use a separate Vulkan pipeline-layout
						 * structure and therefore do not belong in a descriptor
						 * set layout.
						 */
						if ( bindingType == slang::BindingType::PushConstant )
							continue;


						/*
						 * ParameterBlock should normally never reach this point.
						 *
						 * If it does, that would indicate a Slang layout we do
						 * not understand correctly, because ParameterBlock is
						 * supposed to be exposed through sub-object ranges.
						 */
						if ( bindingType == slang::BindingType::ParameterBlock )
						{
							MW_TRACE(
								"Shader {} exposed ParameterBlock as a direct "
								"descriptor range at set {}; deferring it to "
								"sub-object reflection",
								m_Path.string(),
								set
							);

							continue;
						}


						const VkDescriptorType descriptorType =
							getDescriptorType( bindingType );


						if ( descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
						{
							MW_TRACE(
								"Skipping unsupported direct Slang binding type "
								"{} in shader {} set {} range {}",
								static_cast< int >( bindingType ),
								m_Path.string(),
								set,
								rangeIndex
							);

							continue;
						}


						const SlangInt binding =
							pTypeLayout->getDescriptorSetDescriptorRangeIndexOffset(
								relativeSet,
								rangeIndex
							);


						if ( binding < 0 )
						{
							MW_ERROR(
								"Shader {} contains a descriptor range at set {} "
								"whose Vulkan binding index is unresolved",
								m_Path.string(),
								set
							);

							continue;
						}


						const SlangInt descriptorCount =
							pTypeLayout->getDescriptorSetDescriptorRangeDescriptorCount(
								relativeSet,
								rangeIndex
							);


						uint32_t vkDescriptorCount = 1;
						VkDescriptorBindingFlags bindingFlags = 0;


						if ( descriptorCount == SLANG_UNBOUNDED_SIZE )
						{
							/*
							 * Slang tells us that the range is unbounded.
							 *
							 * Vulkan requires a finite descriptorCount in the
							 * layout, together with the VARIABLE_DESCRIPTOR_COUNT
							 * binding flag. A runtime maximum is normally chosen
							 * by the descriptor allocator.
							 *
							 * We use 1 as the conservative layout maximum here,
							 * preserving the variable-binding semantics without
							 * inventing an engine-wide bindless limit inside the
							 * shader reflection layer.
							 */
							vkDescriptorCount = 1;

							bindingFlags |=
								VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;


							MW_TRACE(
								"Shader {} direct descriptor set {} binding {} "
								"is unbounded; using variable descriptor "
								"binding semantics",
								m_Path.string(),
								set,
								binding
							);
						}
						else if ( descriptorCount > 0 )
						{
							vkDescriptorCount =
								static_cast< uint32_t >( descriptorCount );
						}
						else
						{
							/*
							 * Reflection should not normally produce zero here.
							 * Keeping the binding at one descriptor is safer than
							 * emitting an invalid Vulkan layout.
							 */
							vkDescriptorCount = 1;
						}


						slang::VariableReflection* pLeafVariable =
							pTypeLayout->getBindingRangeLeafVariable(
								pTypeLayout->getFieldBindingRangeOffset( rangeIndex )
							);


						const char* debugName =
							pLeafVariable
							? pLeafVariable->getName()
							: nullptr;


						addDescriptor(
							static_cast< uint32_t >( set ),
							static_cast< uint32_t >( binding ),
							vkDescriptorCount,
							descriptorType,
							stageFlags,
							bindingFlags,
							debugName
						);
					}
				}
			};


		/*
		 * -------------------------------------------------------------------------
		 * ParameterBlock reflection
		 * -------------------------------------------------------------------------
		 *
		 * A ParameterBlock contributes:
		 *
		 *     - one independent descriptor set
		 *     - descriptor ranges for the element type inside that set
		 *     - potentially more descriptor sets for nested ParameterBlocks
		 *
		 * The ParameterBlock container itself is NOT emitted as a descriptor
		 * binding in its parent's set.
		 *
		 * For the element type we intentionally use getBindingRangeCount() rather
		 * than getDescriptorSetCount().
		 *
		 * The element is a normal type layout. Slang's own documentation recommends
		 * enumerating its binding ranges and assigning the bindings sequentially.
		 * This also accounts for the implicit constant buffer that Slang introduces
		 * when a ParameterBlock element contains ordinary data.
		 */
		std::function<
			void(
				slang::TypeLayoutReflection*,
				uint32_t,
				VkShaderStageFlags,
				std::string_view
				)
		> reflectParameterBlocks;


		reflectParameterBlocks =
			[&](
				slang::TypeLayoutReflection* pParentTypeLayout,
				uint32_t baseSet,
				VkShaderStageFlags stageFlags,
				std::string_view parentName
				)
			{
				if ( !pParentTypeLayout || stageFlags == 0 )
					return;


				const SlangInt subObjectRangeCount =
					pParentTypeLayout->getSubObjectRangeCount();


				MW_TRACE(
					"Reflecting {} sub-object ranges below '{}' in shader {}",
					subObjectRangeCount,
					parentName.empty() ? "<root>" : parentName,
					m_Path.string()
				);


				for (
					SlangInt subObjectRangeIndex = 0;
					subObjectRangeIndex < subObjectRangeCount;
					++subObjectRangeIndex
					)
				{
					const SlangInt bindingRangeIndex =
						pParentTypeLayout->getSubObjectRangeBindingRangeIndex(
							subObjectRangeIndex
						);


					if ( bindingRangeIndex < 0 )
					{
						MW_ERROR(
							"Shader {} returned invalid binding-range index {} "
							"for sub-object range {}",
							m_Path.string(),
							bindingRangeIndex,
							subObjectRangeIndex
						);

						continue;
					}


					const slang::BindingType bindingType =
						pParentTypeLayout->getBindingRangeType(
							bindingRangeIndex
						);

					slang::VariableLayoutReflection* pSubObjectLayout =
						pParentTypeLayout->getSubObjectRangeOffset(
							subObjectRangeIndex
						);

					if ( !pSubObjectLayout )
					{
						MW_ERROR(
							"Shader {} contains a ParameterBlock sub-object without "
							"a variable layout",
							m_Path.string()
						);

						continue;
					}

					const size_t setOffset =
						pSubObjectLayout->getOffset(
							slang::ParameterCategory::SubElementRegisterSpace
						);

					if ( setOffset == SLANG_UNKNOWN_SIZE )
					{
						MW_ERROR(
							"Shader {} contains a ParameterBlock '{}' whose descriptor "
							"set is unresolved",
							m_Path.string(),
							pSubObjectLayout->getName()
							? pSubObjectLayout->getName()
							: "<unnamed>"
						);

						continue;
					}

					const uint32_t set =
						baseSet +
						static_cast< uint32_t >( setOffset );


					if ( bindingType != slang::BindingType::ParameterBlock )
					{
						/*
						 * Sub-object ranges are not necessarily all ParameterBlocks
						 * (existential values and other Slang sub-objects also use
						 * this mechanism). We only turn ParameterBlock ranges into
						 * Vulkan descriptor sets.
						 */
						MW_TRACE(
							"Skipping non-ParameterBlock sub-object range {} "
							"(bindingType = {}) in shader {}",
							subObjectRangeIndex,
							static_cast< int >( bindingType ),
							m_Path.string()
						);

						continue;
					}


					if ( !ensureSet( static_cast< SlangInt >( set ) ) )
						continue;


					slang::TypeLayoutReflection* pParameterBlockTypeLayout =
						pParentTypeLayout->getBindingRangeLeafTypeLayout(
							bindingRangeIndex
						);


					if ( !pParameterBlockTypeLayout )
					{
						MW_ERROR(
							"Shader {} contains a ParameterBlock sub-object "
							"without a valid type layout",
							m_Path.string()
						);

						continue;
					}


					slang::TypeLayoutReflection* pElementTypeLayout =
						pParameterBlockTypeLayout->getElementTypeLayout();


					if ( !pElementTypeLayout )
					{
						MW_ERROR(
							"Shader {} contains a ParameterBlock sub-object "
							"without a valid element type layout",
							m_Path.string()
						);

						continue;
					}


					slang::VariableLayoutReflection* pSubObjectOffset =
						pParentTypeLayout->getSubObjectRangeOffset(
							subObjectRangeIndex
						);


					const char* parameterBlockName =
						pSubObjectOffset
						? pSubObjectOffset->getName()
						: nullptr;


					MW_TRACE(
						"Found ParameterBlock '{}' in shader {}: "
						"set = {},"
						"elementBindingRanges = {}, stageFlags = 0x{:X}",
						parameterBlockName ? parameterBlockName : "<unnamed>",
						m_Path.string(),
						set,
						pElementTypeLayout->getBindingRangeCount(),
						stageFlags
					);


					/*
					 * Reflect the descriptor ranges belonging to the element.
					 *
					 * Each valid range consumes one binding in the ParameterBlock's
					 * descriptor set.
					 *
					 * ParameterBlock sub-ranges are deliberately skipped here and
					 * recursively handled after the direct element descriptors.
					 */
					uint32_t nextBinding = 0;


					const SlangInt bindingRangeCount =
						pElementTypeLayout->getBindingRangeCount();


					for (
						SlangInt rangeIndex = 0;
						rangeIndex < bindingRangeCount;
						++rangeIndex
						)
					{
						const slang::BindingType elementBindingType =
							pElementTypeLayout->getBindingRangeType(
								rangeIndex
							);


						if (
							elementBindingType ==
							slang::BindingType::ParameterBlock
							)
						{
							/*
							 * Nested ParameterBlock:
							 *
							 * It is NOT a descriptor binding in the current
							 * descriptor set. Its own descriptor set is discovered
							 * through the recursive sub-object pass below.
							 */
							MW_TRACE(
								"ParameterBlock '{}' contains nested ParameterBlock "
								"range {}. Deferring to recursive sub-object "
								"reflection",
								parameterBlockName
								? parameterBlockName
								: "<unnamed>",
								rangeIndex
							);

							continue;
						}


						if (
							elementBindingType ==
							slang::BindingType::PushConstant
							)
						{
							/*
							 * Push constants belong to the pipeline layout and not
							 * to a descriptor set layout.
							 */
							continue;
						}


						const VkDescriptorType descriptorType =
							getDescriptorType(
								elementBindingType
							);


						if ( descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
						{
							MW_TRACE(
								"Skipping unsupported ParameterBlock '{}' "
								"binding range {} of type {} in shader {}",
								parameterBlockName
								? parameterBlockName
								: "<unnamed>",
								rangeIndex,
								static_cast< int >( elementBindingType ),
								m_Path.string()
							);

							continue;
						}


						const SlangInt descriptorCount =
							pElementTypeLayout->getBindingRangeBindingCount(
								rangeIndex
							);


						uint32_t vkDescriptorCount = 1;
						VkDescriptorBindingFlags bindingFlags = 0;


						if ( descriptorCount == SLANG_UNBOUNDED_SIZE )
						{
							vkDescriptorCount = 1;

							bindingFlags |=
								VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
						}
						else if ( descriptorCount > 0 )
						{
							vkDescriptorCount =
								static_cast< uint32_t >( descriptorCount );
						}
						else
						{
							vkDescriptorCount = 1;
						}


						slang::VariableReflection* pLeafVariable =
							pElementTypeLayout->getBindingRangeLeafVariable(
								rangeIndex
							);


						const char* debugName =
							pLeafVariable
							? pLeafVariable->getName()
							: nullptr;


						addDescriptor(
							set,
							nextBinding,
							vkDescriptorCount,
							descriptorType,
							stageFlags,
							bindingFlags,
							debugName
						);


						MW_TRACE(
							"ParameterBlock '{}' descriptor '{}' -> "
							"set {} binding {}",
							parameterBlockName
							? parameterBlockName
							: "<unnamed>",
							debugName
							? debugName
							: "<unnamed>",
							set,
							nextBinding
						);


						++nextBinding;
					}


					/*
					 * Finally recurse into the element layout's own sub-object
					 * ranges so nested ParameterBlocks become additional Vulkan
					 * descriptor sets.
					 */
					reflectParameterBlocks(
						pElementTypeLayout,
						set,
						stageFlags,
						parameterBlockName
						? parameterBlockName
						: "<unnamed>"
					);
				}
			};


		/*
		 * -------------------------------------------------------------------------
		 * Push constant reflection
		 * -------------------------------------------------------------------------
		 *
		 * PushConstant binding ranges are not descriptor-set bindings.
		 *
		 * Slang's typical push-constant layout starts at byte offset zero for the
		 * program-level range, which is what the existing Monoworks reflection
		 * structure expects.
		 */
			auto reflectPushConstants =
				[&](
					slang::TypeLayoutReflection* pTypeLayout,
					VkShaderStageFlags stageFlags
					)
				{
					if ( !pTypeLayout || stageFlags == 0 )
						return;


					const SlangInt bindingRangeCount =
						pTypeLayout->getBindingRangeCount();


					for (
						SlangInt rangeIndex = 0;
						rangeIndex < bindingRangeCount;
						++rangeIndex
						)
					{
						const slang::BindingType bindingType =
							pTypeLayout->getBindingRangeType(
								rangeIndex
							);


						if (
							bindingType !=
							slang::BindingType::PushConstant
							)
						{
							continue;
						}

						/*
						 * A PushConstant binding range points at the
						 * ConstantBuffer<T> layout.
						 *
						 * The ConstantBuffer itself is only the container.
						 * The actual byte size required by Vulkan is the size
						 * of its element type T.
						 */
						slang::TypeLayoutReflection* pConstantBufferTypeLayout =
							pTypeLayout->getBindingRangeLeafTypeLayout(
								rangeIndex
							);


						if ( !pConstantBufferTypeLayout )
						{
							MW_ERROR(
								"Shader {} contains a push-constant range without "
								"a valid ConstantBuffer type layout",
								m_Path.string()
							);

							continue;
						}


						slang::TypeLayoutReflection* pElementTypeLayout =
							pConstantBufferTypeLayout->getElementTypeLayout();


						if ( !pElementTypeLayout )
						{
							MW_ERROR(
								"Shader {} contains a push-constant ConstantBuffer "
								"without a valid element type layout",
								m_Path.string()
							);

							continue;
						}


						const size_t elementSize =
							pElementTypeLayout->getSize();


						if (
							elementSize == 0 ||
							elementSize == SLANG_UNKNOWN_SIZE
							)
						{
							MW_TRACE(
								"Shader {} push-constant range {} has no resolvable "
								"element size",
								m_Path.string(),
								rangeIndex
							);

							continue;
						}


						if (
							elementSize >
							static_cast< size_t >(
								UINT32_MAX
								)
							)
						{
							MW_ERROR(
								"Shader {} push-constant range {} has an element "
								"size of {} bytes, which exceeds uint32_t",
								m_Path.string(),
								rangeIndex,
								elementSize
							);

							continue;
						}


						addPushConstant(
							0,
							static_cast< uint32_t >( elementSize ),
							stageFlags
						);


						MW_TRACE(
							"Reflected push constant in shader {}: "
							"elementSize = {} bytes, offset = 0, stageFlags = 0x{:X}",
							m_Path.string(),
							elementSize,
							stageFlags
						);
					}
				};


		/*
		 * -------------------------------------------------------------------------
		 * Reflect globals
		 * -------------------------------------------------------------------------
		 */
		slang::VariableLayoutReflection* pGlobalParams =
			pLayout->getGlobalParamsVarLayout();


		if ( pGlobalParams )
		{
			slang::TypeLayoutReflection* pGlobalTypeLayout =
				pGlobalParams->getTypeLayout();


			if ( pGlobalTypeLayout )
			{
				MW_TRACE(
					"Reflecting global parameters of shader {}",
					m_Path.string()
				);


				/*
				 * Direct global resources belong to the program's normal/default
				 * descriptor set(s).
				 *
				 * ParameterBlocks are NOT consumed here. They are found through
				 * getSubObjectRangeCount() below.
				 */
				reflectType(
					pGlobalTypeLayout,
					VK_SHADER_STAGE_ALL
				);


				reflectPushConstants(
					pGlobalTypeLayout,
					VK_SHADER_STAGE_ALL
				);


				reflectParameterBlocks(
					pGlobalTypeLayout,
					0,
					VK_SHADER_STAGE_ALL,
					"global"
				);
			}
		}
		else
		{
			MW_TRACE(
				"Shader {} has no reflected global parameter layout",
				m_Path.string()
			);
		}


		/*
		 * -------------------------------------------------------------------------
		 * Reflect entry points
		 * -------------------------------------------------------------------------
		 */
		const SlangInt entryPointCount =
			pLayout->getEntryPointCount();


		MW_TRACE(
			"Reflecting {} entry points of shader {}",
			entryPointCount,
			m_Path.string()
		);


		for (
			SlangInt entryPointIndex = 0;
			entryPointIndex < entryPointCount;
			++entryPointIndex
			)
		{
			slang::EntryPointLayout* pEntryPoint =
				pLayout->getEntryPointByIndex(
					entryPointIndex
				);


			if ( !pEntryPoint )
			{
				MW_ERROR(
					"Shader {} returned null EntryPointLayout at index {}",
					m_Path.string(),
					entryPointIndex
				);

				continue;
			}


			const SlangStage slangStage =
				pEntryPoint->getStage();


			const VkShaderStageFlags stageFlags =
				getVkShaderStage(
					slangStage
				);


			if ( !stageFlags )
			{
				MW_TRACE(
					"Skipping unsupported Slang entry-point stage {} "
					"for shader {}",
					static_cast< int >( slangStage ),
					m_Path.string()
				);

				continue;
			}


			slang::TypeLayoutReflection* pEntryPointTypeLayout =
				pEntryPoint->getTypeLayout();


			if ( !pEntryPointTypeLayout )
			{
				MW_ERROR(
					"Shader {} entry point {} has no type layout",
					m_Path.string(),
					entryPointIndex
				);

				continue;
			}


			MW_TRACE(
				"Reflecting entry point {} ({}) of shader {}",
				entryPointIndex,
				getStageName( slangStage ),
				m_Path.string()
			);


			/*
			 * Direct entry-point parameters.
			 *
			 * Just like globals, this only walks the sets that belong directly to
			 * the entry-point type layout.
			 */
			reflectType(
				pEntryPointTypeLayout,
				stageFlags
			);


			reflectPushConstants(
				pEntryPointTypeLayout,
				stageFlags
			);


			/*
			 * Entry-point ParameterBlocks are sub-objects and therefore need this
			 * separate traversal.
			 */
			reflectParameterBlocks(
				pEntryPointTypeLayout,
				0,
				stageFlags,
				"entry-point"
			);
		}


		/*
		 * -------------------------------------------------------------------------
		 * Guarantee descriptor set 0
		 * -------------------------------------------------------------------------
		 *
		 * We created index 0 above before reflection, so this is already guaranteed.
		 *
		 * If no ordinary global bindings exist, the result will therefore contain
		 * an empty set-0 layout unless Slang itself assigned some other reflected
		 * object to Vulkan set 0.
		 *
		 * In particular, a shader consisting exclusively of ParameterBlocks may
		 * legitimately have its first ParameterBlock in set 0. This is the actual
		 * compiled shader contract and must not be artificially shifted.
		 */
		if ( descriptorSetBindings.empty() )
			descriptorSetBindings.resize( 1 );


		MW_TRACE(
			"Shader {} reflection discovered {} Vulkan descriptor-set slots",
			m_Path.string(),
			descriptorSetBindings.size()
		);


		/*
		 * -------------------------------------------------------------------------
		 * Sort descriptor bindings
		 * -------------------------------------------------------------------------
		 */
		for ( size_t setIndex = 0; setIndex < descriptorSetBindings.size(); ++setIndex )
		{
			auto& bindings =
				descriptorSetBindings[setIndex];


			std::sort(
				bindings.begin(),
				bindings.end(),
				[]( const DescriptorBinding& lhs, const DescriptorBinding& rhs )
				{
					return lhs.Binding.binding <
						rhs.Binding.binding;
				}
			);
		}


		/*
		 * -------------------------------------------------------------------------
		 * Validate and create Vulkan descriptor-set layouts
		 * -------------------------------------------------------------------------
		 */
		std::vector<VkDescriptorSetLayout> vkSetLayouts;

		vkSetLayouts.resize(
			descriptorSetBindings.size(),
			VK_NULL_HANDLE
		);


		for (
			size_t setIndex = 0;
			setIndex < descriptorSetBindings.size();
			++setIndex
			)
		{
			auto& reflectedBindings =
				descriptorSetBindings[setIndex];


			std::vector<VkDescriptorSetLayoutBinding> bindings;
			std::vector<VkDescriptorBindingFlags> bindingFlags;


			bindings.reserve(
				reflectedBindings.size()
			);

			bindingFlags.reserve(
				reflectedBindings.size()
			);


			for ( const DescriptorBinding& reflectedBinding : reflectedBindings )
			{
				bindings.push_back(
					reflectedBinding.Binding
				);

				bindingFlags.push_back(
					reflectedBinding.Flags
				);
			}


			/*
			 * Vulkan requires a variable descriptor-count binding to be the binding
			 * with the numerically highest binding number in the descriptor set.
			 */
			for ( size_t i = 0; i < bindingFlags.size(); ++i )
			{
				if (
					( bindingFlags[i] &
						VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT ) == 0
					)
				{
					continue;
				}


				if (
					i + 1 !=
					bindingFlags.size()
					)
				{
					MW_ERROR(
						"Shader {} uses a variable descriptor-count binding in "
						"set {} that is not the last binding",
						m_Path.string(),
						setIndex
					);

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
			}


			MW_TRACE(
				"Creating VkDescriptorSetLayout for shader {} set {} "
				"(bindingCount = {})",
				m_Path.string(),
				setIndex,
				bindings.size()
			);


			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
			bindingFlagsInfo.sType =
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;


			bool hasBindingFlags = false;


			for ( VkDescriptorBindingFlags flags : bindingFlags )
			{
				if ( flags != 0 )
				{
					hasBindingFlags = true;
					break;
				}
			}


			if ( hasBindingFlags )
			{
				bindingFlagsInfo.bindingCount =
					static_cast< uint32_t >( bindingFlags.size() );

				bindingFlagsInfo.pBindingFlags =
					bindingFlags.data();
			}


			VkDescriptorSetLayoutCreateInfo createInfo{};

			createInfo.sType =
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

			createInfo.bindingCount =
				static_cast< uint32_t >( bindings.size() );

			createInfo.pBindings =
				bindings.empty()
				? nullptr
				: bindings.data();

			createInfo.pNext =
				hasBindingFlags
				? &bindingFlagsInfo
				: nullptr;


			VkDescriptorSetLayout layoutHandle =
				VK_NULL_HANDLE;


			const VkResult result =
				vkCreateDescriptorSetLayout(
					device,
					&createInfo,
					CVulkanContext::GetCallbacks(),
					&layoutHandle
				);


			if ( result != VK_SUCCESS )
			{
				MW_ERROR(
					"Failed to create VkDescriptorSetLayout for shader {} "
					"set {}. VkResult = {}",
					m_Path.string(),
					setIndex,
					static_cast< int >( result )
				);


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


			vkSetLayouts[setIndex] =
				layoutHandle;
		}


		/*
		 * -------------------------------------------------------------------------
		 * Sort push constant ranges
		 * -------------------------------------------------------------------------
		 */
		std::sort(
			pushConstantRanges.begin(),
			pushConstantRanges.end(),
			[](
				const VkPushConstantRange& lhs,
				const VkPushConstantRange& rhs
				)
			{
				if ( lhs.offset != rhs.offset )
					return lhs.offset < rhs.offset;

				return lhs.size < rhs.size;
			}
		);

		for ( auto& push : pushConstantRanges )
			if ( push.size % 4 != 0 )
				push.size = ( push.size + 3 ) & ~3u;

		/*
		 * -------------------------------------------------------------------------
		 * Create Vulkan pipeline layout
		 * -------------------------------------------------------------------------
		 *
		 * Because vkSetLayouts contains every set from zero to the highest set,
		 * pDescriptorSignatures will have exactly:
		 *
		 *     highestSet + 1
		 *
		 * entries.
		 */
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

		pipelineLayoutInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount =
			static_cast< uint32_t >(
				vkSetLayouts.size()
				);

		pipelineLayoutInfo.pSetLayouts =
			vkSetLayouts.data();

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


		/*
		 * -------------------------------------------------------------------------
		 * Fill RHI reflection data
		 * -------------------------------------------------------------------------
		 */
		reflectionData.pPipelineSignature =
			reinterpret_cast< RHI::PipelineSignature >(
				pipelineLayout
				);


		/*
		 * This resize is intentional.
		 *
		 * The returned array length is NEVER merely the number of non-empty sets.
		 * It is always the highest Vulkan set number + 1.
		 */
		reflectionData.pDescriptorSignatures.resize(
			vkSetLayouts.size()
		);


		for (
			size_t setIndex = 0;
			setIndex < vkSetLayouts.size();
			++setIndex
			)
		{
			reflectionData.pDescriptorSignatures[setIndex] =
				reinterpret_cast< RHI::DescriptorSignature >(
					vkSetLayouts[setIndex]
					);


			MW_TRACE(
				"Shader {} descriptor-set signature [{}] = {} bindings",
				m_Path.string(),
				setIndex,
				descriptorSetBindings[setIndex].size()
			);
		}


		reflectionData.PushConstantRanges.reserve(
			pushConstantRanges.size()
		);


		for ( const VkPushConstantRange& range : pushConstantRanges )
		{
			PushConstantRanges pushConstant{};

			pushConstant.Offset =
				range.offset;

			pushConstant.Size =
				range.size;


			reflectionData.PushConstantRanges.push_back(
				pushConstant
			);
		}


		MW_TRACE(
			"Completed Slang reflection for shader {}: "
			"descriptorSetCount = {}, pushConstantRangeCount = {}",
			m_Path.string(),
			reflectionData.pDescriptorSignatures.size(),
			reflectionData.PushConstantRanges.size()
		);


		m_ReflectionData = reflectionData;

		return m_ReflectionData;

#else

		MW_ERROR(
			"ReflectOnShader() called without Vulkan support for {}",
			m_Path.string()
		);

		return reflectionData;

#endif
	}


}