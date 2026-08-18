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
		CVulkanGraphicsPipeline( const GraphicsPipelineCreationInfo* pInfo ) NOEXCEPT;
		~CVulkanGraphicsPipeline() NOEXCEPT;

		void Init( const GraphicsPipelineCreationInfo* pInfo ) NOEXCEPT override;
		void Shutdown() override;

		void Invalidate( const GraphicsPipelineCreationInfo* pInfo ) NOEXCEPT override;
	
		NODISCARD VkPipeline* GetVulkanPipeline() NOEXCEPT { return &m_VulkanPipeline; }
		NODISCARD VkPipelineLayout* GetVulkanPipelineLayout ( ) NOEXCEPT { return &m_VulkanPipelineLayout; }

		NODISCARD bool IsCompiled() NOEXCEPT override { return m_IsCompiled; };

	private:
		std::vector<VkPipelineColorBlendAttachmentState> m_ColorAttachmentStates; 
		std::vector<VkDynamicState> m_DynamicStates;

		CVertexLayout m_VertexLayout;
		VkPipelineLayout m_VulkanPipelineLayout;
		VkPipeline m_VulkanPipeline;

		bool m_IsCompiled = false;
	};
}
