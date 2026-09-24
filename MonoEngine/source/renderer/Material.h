#pragma once
#include <common/Base.hh>
#include <boost/unordered_map.hpp>
#include <boost/container/flat_map.hpp>

#include <renderer/Shader.hh>

#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

namespace Monoworks 
{
	constexpr char c_MaterialInputPB[] = "u_MaterialInput";
	constexpr char c_AlbedoMapBinding[] = "AlbedoMap";
	constexpr char c_NormalMapBinding[] = "NormalMap";
	constexpr char c_RoughnessMapBinding[] = "RoughnessMap";
	constexpr char c_EmissiveMapBinding[] = "EmissiveMap";
	constexpr char c_MetallicMapBinding[] = "MetallicMap";
	constexpr char c_OcclusionMapBinding[] = "OcclusionMap";
	constexpr char c_MaterialUBOBinding[] = "UBO";

	constexpr u32  c_InvalidMaterialID = 0b1111111111111111111;

	struct alignas( 16 ) MaterialData 
	{
		Vector	AlbedoFactor;
		Vector	EmissiveColor;
		float	Metallicness;
		float	Roughness;
		float	EmissionFactor;
		float	EnviromentMapRotation; // TODO: Implement
		float	AmbientOcclusionFactor;
		u32		MaterialID = c_InvalidMaterialID; // TODO: Implement
	};

	struct MaterialCreationInfo
	{
		Vector AlbedoFactor;
		Vector EmissiveColor;
		Ref<RHI::ITexture2D> MW_NULLABLE hAlbedoMap = nullptr;
		Ref<RHI::ITexture2D> MW_NULLABLE hNormalMap = nullptr;
		Ref<RHI::ITexture2D> MW_NULLABLE hRoughnessMap = nullptr;
		Ref<RHI::ITexture2D> MW_NULLABLE hMetallicMap = nullptr;
		Ref<RHI::ITexture2D> MW_NULLABLE hOcclusionMap = nullptr;
		Ref<RHI::ITexture2D> MW_NULLABLE hEmissiveMap = nullptr; 
		float EmissionFactor;
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
		MW_NOTHROW void SetEmissiveMap(  Ref<RHI::ITexture2D> hMap )	 NOEXCEPT;
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
		void BindUBO( std::string_view parameterBlockName, std::string_view bindingName, std::span<Ref<RHI::IUniformBuffer>> hUniformBuffer, bool forceRewrite = false );

		/**
		* @brief Bind a uniform buffer located in global scope.
		* @param bindingName: Name of the element to bind inside global scope.
		* @param hUniformBuffer: Reference to the uniform buffer to bind
		* @param forceRewrite: Toggle whether to rewrite the uniform buffer if it's already written.
		*/
		void BindUBO( std::string_view bindingName, std::span<Ref<RHI::IUniformBuffer>, MFIF> hUniformBuffer, bool forceRewrite = false );

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

		void SetShader(	Ref<CShader> hShader ) NOEXCEPT;

	private:
		MW_NOTHROW void UpdateUBO() NOEXCEPT;
		MW_NOTHROW EResult SetTexture( u32 set, u32 binding, Ref<RHI::ITexture2D> hTexture, bool forceRewrite ) NOEXCEPT;


		u32 m_MaterialParameterBlock;
		u32 m_MaterialID; // 19 bits only // TODO: Implement

		boost::unordered_map<std::pair<u32, u32>, bool> m_BindingsWritten;
		std::array<std::vector<RHI::DescriptorHandle>, MFIF> m_hDescriptors;

		MaterialData m_MaterialData;
		std::array<Ref<RHI::IUniformBuffer>, MFIF>	m_hMaterialUniformBuffer;

		// NOTE: Either the standard GPass Pipeline specified by the Frame Graph or if any of the user-specified 
		// shaders (m_hVertexShader/m_hPixelShader) are set a custom pipeline generated from those shaders.
		ShaderReflectionData m_ShaderReflectionData;
		bool m_bUseCustomShader = false;
		Hash::hash_t m_CustomGraphicsPipelineHash;
		Ref<RHI::IGraphicsPipeline> m_hGraphicsPipeline;
		Ref<CShader>				m_hShader;

	};

	class CMaterialTable 
	{
	public:
		CMaterialTable( u32 materialCount = 1 );
		CMaterialTable( Ref<CMaterialTable> other );
		virtual ~CMaterialTable();

		bool HasMaterial( u32 materialIndex ) const { return m_hMaterials.find( materialIndex ) != m_hMaterials.end() }
	private:
		boost::container::flat_map<u32, Ref<CMaterial>> m_hMaterials;
		u32 m_MaterialCount
	};

	class MaterialTable
	{
	public:
		MaterialTable( uint32_t materialCount = 1 );
		MaterialTable( Ref<MaterialTable> other );
		~MaterialTable() = default;

		bool HasMaterial( uint32_t materialIndex ) const { return m_Materials.find( materialIndex ) != m_Materials.end(); }
		void SetMaterial( uint32_t index, Ref<Material> material );
		void ClearMaterial( uint32_t index );

		Ref<Material> GetMaterial( uint32_t materialIndex ) const
		{
			VT_CORE_ASSERT( HasMaterial( materialIndex ), "" );
			return m_Materials.at( materialIndex );
		}
		std::map<uint32_t, Ref<Material>>& GetMaterials() { return m_Materials; }
		const std::map<uint32_t, Ref<Material>>& GetMaterials() const { return m_Materials; }

		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		void SetMaterialCount( uint32_t materialCount ) { m_MaterialCount = materialCount; }

		void Clear();
	private:
		std::map<uint32_t, Ref<Material>> m_Materials;
		uint32_t m_MaterialCount;
	};
}
