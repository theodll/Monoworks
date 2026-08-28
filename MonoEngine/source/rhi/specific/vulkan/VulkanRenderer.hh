#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/IndexBuffer.hh>
#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>
#include <rhi/agnostic/ComputePipeline.hh>
#include <rhi/agnostic/DescriptorManager.hh>

#include <rhi/GraphicsAPI.hh>

#ifdef MW_ENABLE_MANUAL_RENDERDOC
#include <renderdoc_app.h>
#endif 

namespace Monoworks::RHI
{
    struct BeginRenderingInfo 
    {

    };

    class CVulkanRenderer final : public IGraphicsAPI
    {
        MW_NOTHROW void Init() NOEXCEPT override;
        MW_NOTHROW void Shutdown() NOEXCEPT override;
        
        // todo add push constants
        MW_NOTHROW void DispatchCompute( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1, DescriptorHandle* pDesciptors, size_t pDescriptorCount ) NOEXCEPT override;
        MW_NOTHROW void DispatchCompute( Ref<IComputePipeline> hPipeline, Vector workgroup, s32 MW_NULLABLE threadID = -1 ) NOEXCEPT override;


        void BeginRendering() NOEXCEPT override;
        void EndRendering() NOEXCEPT override;

    private:

#ifdef MW_ENABLE_MANUAL_RENDERDOC
		RENDERDOC_API_1_1_2* m_RenderDocAPI = nullptr;
#endif 

        Ref<IVertexBuffer> m_Vertices;
        Ref<IIndexBuffer> m_Indices;
        Ref<IGraphicsPipeline> m_Pipeline;
    };
}