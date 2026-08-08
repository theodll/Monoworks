#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/IndexBuffer.hh>
#include <rhi/agnostic/VertexBuffer.hh>
#include <rhi/agnostic/GraphicsPipeline.hh>

#include <rhi/GraphicsAPI.hh>

#ifdef MW_ENABLE_MANUAL_RENDERDOC
#include <renderdoc_app.h>
#endif 

namespace Monoworks::RHI
{
    class CVulkanRenderer : public IGraphicsAPI
    {
        virtual void Init() NOEXCEPT override;
        virtual void Shutdown() NOEXCEPT override;

        virtual void BeginRendering() NOEXCEPT override;
        virtual void EndRendering() NOEXCEPT override;

    private:

#ifdef MW_ENABLE_MANUAL_RENDERDOC
		RENDERDOC_API_1_1_2* m_RenderDocAPI = nullptr;
#endif 

        Ref<IVertexBuffer> m_Vertices;
        Ref<IIndexBuffer> m_Indices;
        Ref<IGraphicsPipeline> m_Pipeline;
    };
}