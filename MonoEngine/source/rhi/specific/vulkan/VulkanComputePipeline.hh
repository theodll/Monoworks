#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/ComputePipeline.hh>

#include <volk/volk.h>

namespace Monoworks::RHI 
{
	class CVulkanComputePipeline : public IComputePipeline
	{
	public:
		CVulkanComputePipeline( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT;
		~CVulkanComputePipeline() NOEXCEPT;

		virtual void Init( const ComputePipelineCreationInfo* pInfo ) override;
		virtual void Shutdown() NOEXCEPT override;

		virtual EResult Invalidate( const ComputePipelineCreationInfo* pInfo ) override;

		NODISCARD virtual bool IsCompiled() NOEXCEPT override { return m_IsCompiled; };

		NODISCARD virtual PipelineSignature* GetSignature() NOEXCEPT { ( PipelineSignature* )&m_VulkanPipelineLayout; };

		NODISCARD VkPipeline*		GetVulkanPipeline()				NOEXCEPT { return &m_VulkanPipeline; };
		NODISCARD VkPipelineLayout* GetVulkanPipelineSignature()	NOEXCEPT { return &m_VulkanPipelineLayout; };

	private:
		VkPipeline m_VulkanPipeline;
		VkPipelineLayout m_VulkanPipelineLayout;

		bool m_IsCompiled = false;
	};

}
