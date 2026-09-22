#include <mwpch.hh>

#include <renderer/Material.h>
#include <renderer/FrameManager.hh>
#include <renderer/FrameGraph.hh>


namespace Monoworks 
{
	using namespace RHI;

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

	NODISCARD static std::expected<u32, EResult> FindParameterBlockNumberByString( std::string_view parameterBlockName, slang::ProgramLayout* pLayout )
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

		return setIndex;
	}

	NODISCARD static std::expected<u32, EResult> FindBindingNumberByString( u32 paramterBlock, std::string_view name, slang::ProgramLayout* pLayout ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		slang::VariableLayoutReflection* pBlockVar = pLayout->getParameterByIndex( paramterBlock );

		if ( pBlockVar == nullptr )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

		slang::TypeLayoutReflection* pBlockTypeLayout = pBlockVar->getTypeLayout();

		if ( pBlockTypeLayout->getKind() != slang::TypeReflection::Kind::ParameterBlock )
			return std::unexpected( MW_ERROR_NON_EXISTANT );

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

	MW_NOTHROW EResult CMaterial::SetTexture( u32 set, u32 binding, Ref<RHI::ITexture2D> hTexture, bool forceRewrite ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		if ( !hTexture )
			return MW_ERROR_INVALID_PARAMETER;

		if ( !m_hCustomShader )
			return MW_ERROR_INVALID_SETUP;

		// TODO: 
		auto reflectionData = m_hCustomShader->ReflectOnShader();
		if ( !reflectionData.pDescriptorSignatures[set] )
		{
			MW_ERROR( "Reflection of Shader did not yield a signature for parameter block at index {}.", set );
			return MW_ERROR_NON_EXISTANT;
		}

		bool rewrite = forceRewrite || !m_BindingsWritten[{set, binding}];

		for ( auto i{ 0uz }; i < MFIF; i++ )
		{
			if ( m_hDescriptors[i].size() <= set )
				m_hDescriptors[i].resize( set + 1 );

			if ( !m_hDescriptors[i][set] )
				m_hDescriptors[i][set] = CDescriptorManager::Allocate( reflectionData.pDescriptorSignatures[set] );

			if ( rewrite )
				CDescriptorManager::WriteImage( m_hDescriptors[i][set], set, hTexture );
		}

		if ( rewrite )
			m_BindingsWritten[{set, binding}] = true;

	}

	CMaterial::CMaterial()
	{
		MW_PROFILE_FUNC;
	}

	CMaterial::CMaterial( const MaterialCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
		m_hShader = CFrameManager::GetCurrentFrameGraph()->GetDefaultBasePassShader();
		auto pb = FindParameterBlockNumberByString( c_MaterialInputPB, m_hShader->GetShaderProgram()->getLayout() );
		
		if ( pb )
			m_MaterialParameterBlock = pb.value();
		else if ( pb.error() == MW_ERROR_NON_EXISTANT )
			MW_API_ERROR( "Failed to find material input parameter block: Non existant." );
		else
			MW_API_ERROR( "Failed to find material input parameter block: Unkown error." );

		

	}


	CMaterial::~CMaterial()
	{
		MW_PROFILE_FUNC;
	}



	MW_NOTHROW void CMaterial::SetAlbedoFactor( const Vector& rFactor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_MaterialData.AlbedoFactor = rFactor;
		UpdateUBO();
	}


	MW_NOTHROW void CMaterial::SetEmissiveColor( const Vector& rEmissiveColor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_MaterialData.EmissiveColor = rEmissiveColor;
		UpdateUBO();
	}


	MW_NOTHROW void CMaterial::SetMetallicness( float metallic ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_MaterialData.Metallicness = metallic;
		UpdateUBO();
	}


	MW_NOTHROW void CMaterial::SetRoughness( float roughness ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_MaterialData.Roughness = roughness;
		UpdateUBO();
	}


	MW_NOTHROW void CMaterial::SetAbientOcclusionFactor( float factor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_MaterialData.AmbientOcclusionFactor = factor;
		UpdateUBO();
	}


	MW_NOTHROW void CMaterial::SetAlbedoMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Write Albedo Map for Material {}", m_MaterialID );

		if ( !hMap )
			MW_API_ERROR( "Invalid Reference to Albedo Map passed." ); return;

		auto mapBind = FindBindingNumberByString( m_MaterialParameterBlock, "AlbedoMap", m_hShader->GetShaderProgram()->getLayout() );

		if ( mapBind )
			SetTexture( m_MaterialParameterBlock, mapBind.value(), hMap, true );
		else
			MW_ERROR( "Failed to set Albedo Map for Material {}", m_MaterialID );
	}


	MW_NOTHROW void CMaterial::SetNormalMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Write Normal Map for Material {}", m_MaterialID );

		if ( !hMap )
			MW_API_ERROR( "Invalid Reference to Normal Map passed." ); return;

		auto mapBind = FindBindingNumberByString( m_MaterialParameterBlock, "NormalMap", m_hShader->GetShaderProgram()->getLayout() );

		if ( mapBind )
			SetTexture( m_MaterialParameterBlock, mapBind.value(), hMap, true );
		else
			MW_ERROR( "Failed to set Normal Map for Material {}", m_MaterialID );
	}

	MW_NOTHROW void CMaterial::SetEmissiveMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Write Emissive Map for Material {}", m_MaterialID );

		if ( !hMap )
			MW_API_ERROR( "Invalid Reference to Emissive Map passed." ); return;

		auto mapBind = FindBindingNumberByString( m_MaterialParameterBlock, "EmissiveMap", m_hShader->GetShaderProgram()->getLayout() );

		if ( mapBind )
			SetTexture( m_MaterialParameterBlock, mapBind.value(), hMap, true );
		else
			MW_ERROR( "Failed to set Emissive Map for Material {}", m_MaterialID );
	}

	MW_NOTHROW void CMaterial::SetRoughnessMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Write Roughness Map for Material {}", m_MaterialID );

		if ( !hMap )
			MW_API_ERROR( "Invalid Reference to Roughness Map passed." ); return;

		auto mapBind = FindBindingNumberByString( m_MaterialParameterBlock, "RoughnessMap", m_hShader->GetShaderProgram()->getLayout() );

		if ( mapBind )
			SetTexture( m_MaterialParameterBlock, mapBind.value(), hMap, true );
		else
			MW_ERROR( "Failed to set Roughness Map for Material {}", m_MaterialID );
	}


	MW_NOTHROW void CMaterial::SetMetalllicMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Write Metallic Map for Material {}", m_MaterialID );

		if ( !hMap )
			MW_API_ERROR( "Invalid Reference to Metallic Map passed." ); return;

		auto mapBind = FindBindingNumberByString( m_MaterialParameterBlock, "MetallicMap", m_hShader->GetShaderProgram()->getLayout() );

		if ( mapBind )
			SetTexture( m_MaterialParameterBlock, mapBind.value(), hMap, true );
		else
			MW_ERROR( "Failed to set Metallic Map for Material {}", m_MaterialID );
	}


	MW_NOTHROW void CMaterial::SetOcclusionMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		MW_TRACE( "Write Occlusion Map for Material {}", m_MaterialID );

		if ( !hMap )
			MW_API_ERROR( "Invalid Reference to Occlusion Map passed." ); return;

		auto mapBind = FindBindingNumberByString( m_MaterialParameterBlock, "OcclusionMap", m_hShader->GetShaderProgram()->getLayout() );

		if ( mapBind )
			SetTexture( m_MaterialParameterBlock, mapBind.value(), hMap, true );
		else
			MW_ERROR( "Failed to set Occlusion Map for Material {}", m_MaterialID );
	}


	void CMaterial::BindTexture( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::BindSampler( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::BindUBO( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::BindUBO( std::string_view parameterBlockName, std::string_view bindingName, std::span<Ref<RHI::IUniformBuffer>> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

	}

	void CMaterial::BindUBO( std::string_view bindingName, std::span<Ref<RHI::IUniformBuffer>, MFIF> hUniformBuffer, bool forceRewrite /*= false */ )
	{
		MW_PROFILE_FUNC;

	}

	void CMaterial::SetShader( Ref<CShader> hShader ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

	MW_NOTHROW void CMaterial::UpdateUBO() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


}
