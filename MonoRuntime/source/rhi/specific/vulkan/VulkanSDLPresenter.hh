#pragma once
#include <Monoworks.hh>

#include <rhi/agnostic/Presenter.hh>
#include <rhi/agnostic/Texture.hh>

#include <volk/volk.h>
#include <SDL3/SDL.h>

namespace Monoworks::RHI 
{

	class CVulkanSDLPresenter : public IPresenter 
	{
	public:
		CVulkanSDLPresenter( SExtent2D swapchainExtent, bool vsync, SDL_Window* window ) NOEXCEPT;

		void Init( const IPresentationInitializationInfo* pInfo ) NOEXCEPT override;
		void Init2(  const IPresentationInitialization2Info* pInfo ) NOEXCEPT override;
		void Shutdown() NOEXCEPT;

		bool OnResize( SEvent& event ) override;

		NODISCARD u32 Aquire( const IPresentationAcquisitionInfo* pInfo ) NOEXCEPT override;
		void Present( const IPresentationPresentInfo* pInfo ) NOEXCEPT override;

		NODISCARD const std::vector<Ref<ITexture2D>>& GetSwapchainImages() NOEXCEPT override { return m_SwapchainImages; };
		NODISCARD VkSurfaceKHR* GetSurface() NOEXCEPT { return &m_Surface; };
		NODISCARD VkSwapchainKHR* GetSwapchain() NOEXCEPT { return &m_Swapchain; }
	private:
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_Swapchain;

		SExtent2D m_SwapchainExtent;
		SDL_Window* m_Window;

		bool m_VSync = false;
		u32 m_QueueNodeIndex = UINT32_MAX;

		u32 m_CurrentImageIndex = 0;

		std::vector<Ref<ITexture2D>> m_SwapchainImages;

		std::vector<u32> m_ImagePresentedOnce;

		VkFormat m_ColorFormat;
		VkColorSpaceKHR m_ColorSpace;
	};
}
