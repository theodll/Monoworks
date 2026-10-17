#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/Texture.hh>
#include <rhi/agnostic/UniformBuffer.hh>

#include <renderer/Shader.hh>

namespace Monoworks::RHI 
{
	using DescriptorHandle = void*; 

	class CDescriptorManager 
	{
	public:
		static void Init() NOEXCEPT;
		static void Shutdown() NOEXCEPT;

		static void WriteUniformBuffer( IN DescriptorHandle hDescriptor, IN u32 binding, IN Ref<IUniformBuffer> hUniformBuffer );

		static void WriteBuffer( IN DescriptorHandle hDescriptor, IN u32 binding, IN void* pBuffer, IN size_t size );
		// Only writes the Image!
		static void WriteImage(	IN DescriptorHandle hDescriptor, IN u32 binding, IN Ref<ITexture2D> hTexture, IN EDescriptorType imageType = MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
		// Only writes the Sampler! 
		static void WriteSampler( IN DescriptorHandle hDescriptor, IN u32 binding, IN Ref<ITexture2D> hTexture );
		NODISCARD static DescriptorHandle Allocate( IN DescriptorSignature MW_NULLABLE hSetLayout, IN u32 MW_NULLABLE maxSetHints = 128 );

		static void DestroyDescriptorSignature( IN DescriptorSignature MW_NULLABLE hSignature ) NOEXCEPT;
		static void DestroyPipelineSignature( IN PipelineSignature MW_NULLABLE hSignature ) NOEXCEPT;

	protected:
		virtual void InitImpl() NOEXCEPT = 0;
		virtual void ShutdownImpl() NOEXCEPT = 0;
		virtual void WriteUniformBufferImpl( DescriptorHandle hDescriptor, u32 binding, Ref<IUniformBuffer> hUniformBuffer ) = 0;
		virtual void WriteBufferImpl( DescriptorHandle hDescriptor, u32 binding, void* pBuffer, size_t size ) = 0;
		virtual void WriteImageImpl( DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture, EDescriptorType imageType = MW_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) = 0;
		virtual void WriteSamplerImpl( DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture ) = 0;
		virtual DescriptorHandle AllocateImpl( DescriptorSignature hSetLayout, u32 maxSetHints MW_NULLABLE = 128 ) = 0;

		virtual void DestroyDescriptorSignatureImpl( IN DescriptorSignature MW_NULLABLE hSignature ) NOEXCEPT = 0;
		virtual void DestroyPipelineSignatureImpl( IN PipelineSignature MW_NULLABLE hSignature ) NOEXCEPT = 0;


	private:
		static Ref<CDescriptorManager> m_hInstance;
	};
}
