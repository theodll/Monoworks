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
		CVulkanGraphicsPipeline( const GraphicsPipelineCreationInfo* pInfo );
		~CVulkanGraphicsPipeline() NOEXCEPT;

		void Init( const GraphicsPipelineCreationInfo* pInfo ) override;
		void Shutdown() override;

		EResult Invalidate( const GraphicsPipelineCreationInfo* pInfo ) override;
	
		NODISCARD bool IsCompiled() NOEXCEPT override { return m_IsCompiled; };

		NODISCARD virtual PipelineSignature* GetSignature() NOEXCEPT { ( PipelineSignature* )&m_VulkanPipelineLayout; };

		NODISCARD VkPipeline* GetVulkanPipeline()				NOEXCEPT { return &m_VulkanPipeline; };
		NODISCARD VkPipelineLayout* GetVulkanPipelineSignature()	NOEXCEPT { return &m_VulkanPipelineLayout; };

	private:
		std::vector<VkPipelineColorBlendAttachmentState> m_ColorAttachmentStates; 
		std::vector<VkDynamicState> m_DynamicStates;

		CVertexLayout m_VertexLayout;

		VkPipelineLayout m_VulkanPipelineLayout;
		VkPipeline m_VulkanPipeline;

		bool m_IsCompiled = false;
	};
}
