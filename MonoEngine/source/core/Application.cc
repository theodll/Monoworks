#include "Application.hh"
#include "CVarManager.hh"
#include "ConfigManager.hh"

#include <renderer/StaticRenderer.hh>

#include <events/EventManager.hh>
#include <events/Event.hh>

#include <common/Events.hh>
#include <common/Base.hh>

#include <functional>
#include <thread>
#include <chrono>
void* operator new(std::size_t count)
{
    MW_PROFILE_FUNC;
    void* pPtr = std::malloc(count);
    if (!pPtr) throw std::bad_alloc();
    MW_PROFILE_ALLOC(pPtr, count);
    return pPtr;
}

void operator delete(void* pPtr) noexcept
{
    MW_PROFILE_FUNC;
    if (pPtr) {
        MW_PROFILE_FREE(pPtr);
        std::free(pPtr);
    }
}

void operator delete(void* pPtr, MAYBE_UNUSED size_t size) noexcept
{
    MW_PROFILE_FUNC;
    if (pPtr) {
        // Tipp: Wenn dein Profiler Size-Aware-Frees unterstützt (z. B. Tracy), 
        // nutze MW_PROFILE_FREE_N(pPtr, size), ansonsten reicht MW_PROFILE_FREE:
        MW_PROFILE_FREE(pPtr);
        std::free(pPtr);
    }
}

void* operator new[](std::size_t count)
{
    MW_PROFILE_FUNC;
    void* pPtr = std::malloc(count);
    if (!pPtr) throw std::bad_alloc();
    MW_PROFILE_ALLOC(pPtr, count);
    return pPtr;
}

void operator delete[](void* pPtr) noexcept
{
    MW_PROFILE_FUNC;
    if (pPtr) {
        MW_PROFILE_FREE(pPtr);
        std::free(pPtr);
    }
}

void operator delete[](void* pPtr, MAYBE_UNUSED size_t size) noexcept
{
    MW_PROFILE_FUNC;
    if (pPtr) {
        MW_PROFILE_FREE(pPtr);
        std::free(pPtr);
    }
}

namespace Monoworks
{
	CApplication* CApplication::m_Singleton;
	Ref<Monoworks::RHI::CVulkanContext> CApplication::m_GraphicsContext;
	SApplicationCreateInfos CApplication::m_pApplicationCreationInfos;
	EGraphicsAPI CApplication::m_GraphicsAPI;

	CApplication::CApplication() noexcept
	{
		MW_PROFILE_FUNC;
		CLogManager::Init();
		CCvarManager::Init();
		CMemoryManager::Init();
		CEventManager::Init();
	}

	CApplication::~CApplication() noexcept
	{
		MW_PROFILE_FUNC;
		CEventManager::Shutdown();
		CMemoryManager::Shutdown();
		CCvarManager::Shutdown();
		CLogManager::Shutdown();
	}

	void CApplication::Init(const SApplicationCreateInfos* pInfos) NOEXCEPT
	{
		MW_PROFILE_FUNC;

		m_pApplicationCreationInfos = *pInfos;

		m_GraphicsAPI = pInfos->GraphicsAPI;

		m_GraphicsContext = Ref<RHI::CVulkanContext>::Create();
		m_GraphicsContext->Init();

		CStaticRenderer::Init();
	}

	void CApplication::Shutdown() NOEXCEPT
	{
		MW_PROFILE_FUNC;
		
		CStaticRenderer::Shutdown();

		m_GraphicsContext->Shutdown();
	}

	void CApplication::Frame()
	{
		MW_PROFILE_FUNC;

		// called once per frame
		Events::SAppFrame frame{};
		CEventManager::EmitEventNonDeffered(frame, MW_EVENT_APP_FRAME);

		CEventManager::ProcessEvents();

		Events::SAppTick tick{};
		CEventManager::EmitEventNonDeffered(tick, MW_EVENT_APP_TICK);
		// simulate here

		CStaticRenderer::BeginRendering();

		Events::SAppRender render{};
		CEventManager::EmitEventNonDeffered(render, MW_EVENT_APP_RENDER);

		CStaticRenderer::EndRendering();

		


		FrameMark;
	}
}