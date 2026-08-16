#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>

#include <volk/volk.h>

namespace Monoworks::RHI 
{

	class CVulkanGraphicsPipeline : public IGraphicsPipeline
	{
	public:
		CVulkanGraphicsPipeline( const SGraphicsPipelineCreationInfo* pInfo ) NOEXCEPT;
		~CVulkanGraphicsPipeline() NOEXCEPT;

		void Init( const SGraphicsPipelineCreationInfo* pInfo ) NOEXCEPT override;
		void Shutdown() override;

		void Invalidate( const SGraphicsPipelineCreationInfo* pInfo ) NOEXCEPT override;
	
		NODISCARD VkPipeline* GetVulkanPipeline() NOEXCEPT { return &m_VulkanPipeline; }
		NODISCARD VkPipelineLayout* GetVulkanPipelineLayout ( ) NOEXCEPT { return &m_VulkanPipelineLayout; }

	private:
		std::vector<VkPipelineColorBlendAttachmentState> m_ColorAttachmentStates; 
		std::vector<VkDynamicState> m_DynamicStates;

		CVertexLayout m_VertexLayout;
		VkPipelineLayout m_VulkanPipelineLayout;
		VkPipeline m_VulkanPipeline;
	};
}
