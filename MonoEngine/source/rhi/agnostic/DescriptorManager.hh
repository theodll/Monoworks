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

		static void WriteUniformBuffer( DescriptorHandle hDescriptor, u32 binding, Ref<IUniformBuffer> hUniformBuffer );

		static void WriteBuffer(	DescriptorHandle hDescriptor, u32 binding, void* pBuffer, size_t size );
		// Only writes the Image!
		static void WriteImage(	DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture );
		// Only writes the Sampler! 
		static void WriteSampler(	DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture );
		NODISCARD static DescriptorHandle Allocate( DescriptorSignature hSetLayout, u32 maxSetHints MW_NULLABLE = 128 );

	protected:
		virtual void InitImpl() NOEXCEPT = 0;
		virtual void ShutdownImpl() NOEXCEPT = 0;
		virtual void WriteUniformBufferImpl( DescriptorHandle hDescriptor, u32 binding, Ref<IUniformBuffer> hUniformBuffer ) = 0;
		virtual void WriteBufferImpl( DescriptorHandle hDescriptor, u32 binding, void* pBuffer, size_t size );
		virtual void WriteImageImpl( DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture );
		virtual void WriteSamplerImpl( DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture );
		virtual DescriptorHandle AllocateImpl( DescriptorSignature hSetLayout, u32 maxSetHints MW_NULLABLE = 128 );

	private:
		static Ref<CDescriptorManager> m_hInstance;
	};
}
