#pragma once
#include <common/Base.hh>
#include <common/Memory.hh>
#include <common/Math.hh>

#include <rhi/GraphicsAPI.hh>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Monoworks 
{
    class CStaticRenderer
    {
    public:
        static void Init() NOEXCEPT;
        static void Shutdown() NOEXCEPT;

        static void BeginRendering() NOEXCEPT;
        static void EndRendering() NOEXCEPT;

        NODISCARD static u32  GetCurrentFrameIndex() NOEXCEPT { return m_CurrentFrameIndex; };
        NODISCARD static u32* GetCurrentFrameIndexPtr() NOEXCEPT { return &m_CurrentFrameIndex; };

		NODISCARD static u32  GetCurrentImageIndex() NOEXCEPT { return m_CurrentImageIndex; };
		NODISCARD static u32* GetCurrentImageIndexPtr() NOEXCEPT { return &m_CurrentImageIndex; };

        NODISCARD static Slang::ComPtr<slang::IGlobalSession> GetSlangGlobalSession() NOEXCEPT { return m_SlangGlobalSession; };
    private:
        static u32 m_CurrentFrameIndex;
        static u32 m_CurrentImageIndex;

        static Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;

        static Ref<RHI::IGraphicsAPI> m_pInstance;
    };
}