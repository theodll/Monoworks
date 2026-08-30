#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>

#include <renderer/Shader.hh>

#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

namespace Monoworks 
{

	struct alignas( 16 ) MaterialData 
	{
		Vector	AlbedoFactor;
		Vector	EmissiveColor;
		float	Metallicness;
		float	Roughness;
		float	EmissionFactor;
		float	EnviromentMapRotation; // TODO: Implement
		float	AmbientOcclusionFactor;
		bool	UseNormalMap; 
		const char	_pad[3];
	};

	struct MaterialCreationInfo
	{
		Vector AlbedoFactor;
		Vector EmissiveColor;
		Ref<RHI::ITexture2D> hAlbedoMap;
		Ref<RHI::ITexture2D> hNormalMap;
		Ref<RHI::ITexture2D> hRoughnessMap;
		Ref<RHI::ITexture2D> hMetallicMap;
		Ref<RHI::ITexture2D> hOcclusionMap; 
		float Metallicness;
		float Roughness;
		float AmbientOcclusion;
	};

	class CMaterial 
	{
	public:
		CMaterial();
		CMaterial( const MaterialCreationInfo* pInfo );
		virtual ~CMaterial();

		MW_NOTHROW void SetAlbedoFactor( const Vector& rFactor )		 NOEXCEPT;
		MW_NOTHROW void SetEmissiveColor( const Vector& rEmissiveColor ) NOEXCEPT;
		MW_NOTHROW void SetMetallicness( float metallic )				 NOEXCEPT;
		MW_NOTHROW void SetRoughness( float roughness )					 NOEXCEPT;
		MW_NOTHROW void SetAbientOcclusionFactor( float factor )		 NOEXCEPT;

		MW_NOTHROW void SetAlbedoMap(	 Ref<RHI::ITexture2D> hMap )	 NOEXCEPT;
		MW_NOTHROW void SetNormalMap(	 Ref<RHI::ITexture2D> hMap )	 NOEXCEPT;
		MW_NOTHROW void SetRoughnessMap( Ref<RHI::ITexture2D> hMap )	 NOEXCEPT;
		MW_NOTHROW void SetMetalllicMap( Ref<RHI::ITexture2D> hMap )	 NOEXCEPT;
		MW_NOTHROW void SetOcclusionMap( Ref<RHI::ITexture2D> hMap )	 NOEXCEPT;

		/**
		* @brief Bind a texture located in a parameter block.
		* @param parameterBlockName: Name of the parameter block in the shader code.
		* @param bindingName: Name of the element to bind inside the parameter block.
		* @param hTexture: Reference to the texture to bind.
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		void BindTexture( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );

		/**
		* @brief Bind a texture located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hTexture: Reference to the texture to bind
		* @param forceRewrite: Toggle whether to rewrite the texture if it's already written.
		*/
		void BindTexture( std::string_view bindingName, Ref<RHI::ITexture2D> hTexture, bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in a parameter block.
		* @param parameterBlockName: Name of the parameter block in the shader code.
		* @param bindingName: Name of the element to bind inside the Parameter Block.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite = false );

		/**
		* @brief Bind a sampler located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hSampler: Reference to the sampler to bind
		* @param forceRewrite: Toggle whether to rewrite the sampler if it's already written.
		*/
		void BindSampler( std::string_view bindingName, Ref<RHI::ITexture2D> hSampler, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in a parameter block.
		* @param parameterBlockName: Name of the parameter block in the shader code.
		* @param bindingName: Name of the element to bind inside the Parameter Block.
		* @param hUniformBuffer: Reference to the uniform buffer to bind.
		* @param forceRewrite: Toggle whether to rewrite the sampler uniform buffer if it's already written.
		*/
		void BindUBO( std::string_view parameterBlockName, std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffer: Reference to the uniform buffer to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO( std::string_view bindingName, Ref<RHI::IUniformBuffer> hUniformBuffer, bool forceRewrite = false );

		void SetVertexShader(	Ref<CShader> hVertexShader ) NOEXCEPT;
		void SetPixelShader(	Ref<CShader> hPixelShader )	 NOEXCEPT;

	private:
		MW_NOTHROW void UpdateUBO() NOEXCEPT;

		boost::unordered_map<std::pair<u32, u32>, bool> m_BindingsWritten;
		std::vector<std::array<RHI::DescriptorHandle, MFIF>> m_pDescriptors;

		MaterialData m_Data;
		Ref<RHI::IUniformBuffer>	m_hUniformBuffer;

		// NOTE: Either the standard GPass Pipeline specified by the Frame Graph or if any of the user-specified 
		// shaders (m_hVertexShader/m_hPixelShader) are set a custom pipeline generated from those shaders.
		Ref<RHI::IGraphicsPipeline> m_hGraphicsPipeline;
		Ref<CShader>	MW_NULLABLE m_hCustomShader;

	};
}
