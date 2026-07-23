#include "Application.hh"
#include "CVarManager.hh"
#include "ConfigManager.hh"

#include <events/EventManager.hh>
#include <events/Event.hh>

#include <common/Events.h>
#include <common/Base.hh>

#include <functional>
#include <thread>
#include <chrono>

void* operator new(size_t count)
{
	MW_PROFILE_FUNC;
	void* ptr = std::malloc(count);
	if (!ptr) throw std::bad_alloc();
	MW_PROFILE_ALLOC(ptr, count);
	return ptr;
}

void operator delete(void* ptr) noexcept 
{
	MW_PROFILE_FUNC;
	if(ptr)
	{
		MW_PROFILE_FREE(ptr);
		std::free(ptr);
	}
}

void* operator new[](std::size_t count) 
{
	MW_PROFILE_FUNC;
	void* ptr = std::malloc(count);
	if (!ptr) throw std::bad_alloc();
	MW_PROFILE_ALLOC(ptr, count);
	return ptr;
}

void operator delete[](void* ptr) noexcept 
{
	MW_PROFILE_FUNC;
	if (ptr) {
		MW_PROFILE_FREE(ptr);
		std::free(ptr);
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

	void CApplication::Init(const SApplicationCreateInfos* pInfos) noexcept
	{
		MW_PROFILE_FUNC;

		m_pApplicationCreationInfos = *pInfos;

		m_GraphicsContext = Ref<RHI::CVulkanContext>::Create();
		m_GraphicsContext->Init();
	}

	void CApplication::Shutdown() noexcept
	{
		MW_PROFILE_FUNC;
		free((void*)m_pApplicationCreationInfos.pName);

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


		Events::SAppRender render{};
		CEventManager::EmitEventNonDeffered(render, MW_EVENT_APP_RENDER);
		// render here
		std::this_thread::sleep_for(std::chrono::milliseconds(7));


		FrameMark;
	}
}