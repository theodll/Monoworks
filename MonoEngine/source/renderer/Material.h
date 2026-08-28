#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>

#include <renderer/Shader.hh>

#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

namespace Monoworks 
{

	struct alignas( 16 ) MaterialUBO
	{
		Vector4 BaseColorFactor{ 1.0f };
		float Metallic{ 0.0f };
		float Roughness{ 0.0f };
		float AbientOcclusionFactor{ 1.0f };
		float _padding0{ 0.0f };
		Vector EmissiveColor{ 0.0f };
		float _padding1{ 0.0f };
	};


	class CMaterial 
	{
	public:
		CMaterial();
		virtual ~CMaterial();

		MW_NOTHROW void SetBaseColorFactor( const Vector4& rFactor )	NOEXCEPT;
		MW_NOTHROW void SetMetallic( float metallic )					NOEXCEPT;
		MW_NOTHROW void SetRoughness( float roughness )					NOEXCEPT;
		MW_NOTHROW void SetAbientOcclusionFactor( float ao )			NOEXCEPT;
		MW_NOTHROW void SetEmissiveColor( const Vector& emissiveColor ) NOEXCEPT;

		MW_NOTHROW void SetAlbedoTexture( Ref<RHI::ITexture2D> hTexture ) NOEXCEPT;
		MW_NOTHROW void SetNormalTexture( Ref<RHI::ITexture2D> hTexture ) NOEXCEPT;
		MW_NOTHROW void SetRoughnessTexture( Ref<RHI::ITexture2D> hTexture ) NOEXCEPT;
		MW_NOTHROW void SetMetalllicTexture( Ref<RHI::ITexture2D> hTexture2D ) NOEXCEPT;


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


	private:
		MW_NOTHROW void UpdateUBO() NOEXCEPT;

		boost::unordered_map<std::pair<u32, u32>, bool> m_BindingsWritten;
		std::vector<std::array<RHI::DescriptorHandle, MFIF>> m_pDescriptors;

		MaterialUBO m_Data;
		Ref<RHI::IUniformBuffer>	m_hUniformBuffer;

		Ref<RHI::IGraphicsPipeline> m_hGraphicsPipeline;

		// NOTE: Both shaders are independent from each other and can be null.
		// If both are null, the default graphics pipeline provided by the selected frame graph.
		Ref<CShader>	MW_NULLABLE	m_hVertexShader;
		Ref<CShader>	MW_NULLABLE m_hFragmentShader;

	};
}
