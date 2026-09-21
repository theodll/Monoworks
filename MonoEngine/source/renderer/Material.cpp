#include <mwpch.hh>

#include <renderer/Material.h>

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

	NODISCARD static std::expected<u32, EResult> FindBindingNumberByString( std::string_view name, slang::ProgramLayout* pLayout ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
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

	MW_NOTHROW void CMaterial::SetTexture( u32 set, u32 binding, Ref<RHI::ITexture2D> hTexture ) NOEXCEPT
	{
		MW_PROFILE_FUNC;



	}

	CMaterial::CMaterial()
	{
		MW_PROFILE_FUNC;
	}

	CMaterial::CMaterial( const MaterialCreationInfo* pInfo )
	{
		MW_PROFILE_FUNC;
	}


	CMaterial::~CMaterial()
	{
		MW_PROFILE_FUNC;
	}



	MW_NOTHROW void CMaterial::SetAlbedoFactor( const Vector& rFactor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetEmissiveColor( const Vector& rEmissiveColor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetMetallicness( float metallic ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetRoughness( float roughness ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetAbientOcclusionFactor( float factor ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetAlbedoMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetNormalMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetRoughnessMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetMetalllicMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::SetOcclusionMap( Ref<RHI::ITexture2D> hMap ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
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
