#include <mwpch.hh>

#include <core/Application.hh>
#include "DescriptorManager.hh"

#include <rhi/specific/vulkan/VulkanDescriptorSetManager.h>

namespace Monoworks::RHI 
{
	Ref<CDescriptorManager> CDescriptorManager::m_hInstance;

	void CDescriptorManager::Init() NOEXCEPT
	{
		MW_PROFILE_FUNC;

		switch ( CApplication::GetGraphicsAPI() )
		{
		case MW_GAPI_NONE:    m_hInstance = nullptr;
		case MW_GAPI_VULKAN:  m_hInstance = Ref<CVulkanDescriptorSetManager>::Create();
		}
		MW_ASSERT( false, "Unknown Graphics API" );
		
		m_hInstance->Init();

	};

	void CDescriptorManager::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		m_hInstance->Shutdown();

	};

	void CDescriptorManager::WriteBuffer( DescriptorHandle hDescriptor, u32 binding, void* pBuffer, size_t size ) 
	{
		MW_PROFILE_FUNC;
		m_hInstance->WriteBufferImpl( hDescriptor, binding, pBuffer, size );

	};

	void CDescriptorManager::WriteImage( DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture )
	{
		MW_PROFILE_FUNC;
		m_hInstance->WriteImageImpl( hDescriptor, binding, hTexture );
	};

	void CDescriptorManager::WriteSampler( DescriptorHandle hDescriptor, u32 binding, Ref<ITexture2D> hTexture )
	{
		MW_PROFILE_FUNC;
		m_hInstance->WriteSamplerImpl( hDescriptor, binding, hTexture );
	};

	DescriptorHandle CDescriptorManager::Allocate( DescriptorSignature hSetLayout, u32 maxSetHints MW_NULLABLE = 128 ) 
	{
		MW_PROFILE_FUNC;
		return m_hInstance->AllocateImpl( hSetLayout, maxSetHints );
	};

	void CDescriptorManager::WriteUniformBuffer( DescriptorHandle hDescriptor, u32 binding, Ref<IUniformBuffer> hUniformBuffer )
	{
		MW_PROFILE_FUNC;
		return m_hInstance->WriteUniformBufferImpl( hDescriptor, binding, hUniformBuffer );
	}

}
