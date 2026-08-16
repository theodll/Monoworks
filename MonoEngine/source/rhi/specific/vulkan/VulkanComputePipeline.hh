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

		virtual void Init( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT override;
		virtual void Shutdown() NOEXCEPT override;

		virtual void Invalidate( const ComputePipelineCreationInfo* pInfo ) NOEXCEPT override;

	private:
		VkPipeline m_VulkanPipeline;
		VkPipelineLayout m_VulkanPipelineLayout;
	};

}
