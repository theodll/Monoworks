#include <mwpch.hh>

#include <renderer/Material.h>

namespace Monoworks 
{


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


	void CMaterial::SetVertexShader( Ref<CShader> hVertexShader ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	void CMaterial::SetPixelShader( Ref<CShader> hPixelShader ) NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}


	MW_NOTHROW void CMaterial::UpdateUBO() NOEXCEPT
	{
		MW_PROFILE_FUNC;
	}

}
