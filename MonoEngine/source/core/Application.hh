/**
 * @defgroup Core Core
 * @brief Core components of the engine.
 */

/**
 * @file Application.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief CApplication class
 * @version 0.1
 * @date 2026-07-08
 * @copyright Copyright (c) 2026
 * @ingroup Core
 */

#pragma once
#include <common/Base.hh>

#include <rhi/agnostic/Presenter.hh>

#include <rhi/specific/vulkan/VulkanContext.hh>

#include <string>

namespace Monoworks 
{
	/**
	 * @brief All infos the Engine needs to initialize
	 */
	struct SApplicationCreateInfos 
	{	
		/**
		 * @brief Name of the Application the engine is integrated in.
		*/
		MAYBE_UNUSED const char* pName = "MonoEngine";
		/**
		* @brief Array of arguments.
		*/
		MAYBE_UNUSED char** pArguments = nullptr;
		/**
		 * @brief Callback to retrieve
		 */
		MAYBE_UNUSED const char** (*RequiredExtensionCallback)(u32* extensionCount);
		/**
		 * @brief Extent the engine is able to render to (eg. Window/Viewport size)
		 */
		MAYBE_UNUSED SExtent2D RenderableExtent = { 640, 480 };
		/**
		* @brief Presentation Interface.
		*/
		MAYBE_UNUSED RHI::IPresenter* pPresenter;
		/**
		 * @brief Graphics API used by the renderer.
		 */
		MAYBE_UNUSED EGraphicsAPI GraphicsAPI;
		/**
		* @brief Number of items in the Arguments array.
		*/
		MAYBE_UNUSED s32 ArgumentCount = 0;
		/**
		 * @brief Version of the associated Application.
		 */
		MAYBE_UNUSED SAppVersion Version;
		/**
		 * @brief Define whether to use the Vulkan swapchain.
		 */
		MAYBE_UNUSED bool UseSwapchain;
		/**
		 * @brief Define whether to use SDL.
		 */
		MAYBE_UNUSED bool UseSDL;
		/**
		 * @brief Define whether to use Qt.
		 */
		MAYBE_UNUSED bool UseQt;
	};

	/**
	 * @brief Root Engine Class managing the entire engine
	 */
	class CApplication 
	{
	public:
		CApplication() noexcept;
		virtual ~CApplication() noexcept;
		
		/**
		 * @brief Initializes the non "ultra"-core engine, like Rendering and not Memory Allocation or Logging - That is done by the constructor.
		 * @param pInfos All configuration parameters via the SApplicationCreateInfos associated to the engine
		 */
		void Init( const SApplicationCreateInfos* pInfos ) noexcept;

		/**
		 * @brief Shuts the engine down.
		 * Releases all the memory, kills the rendering context and shuts down every subsystem
		 */
		void Shutdown() noexcept;

		/**
		 * @brief Executes exactly one engine frame.
		 * Has to be called in a loop.
		 */
		void Frame();


		/**
		 * @brief Generic Getter for the Application Singleton
		 * @return CApplication* Singleton of the Application
		 */
		NODISCARD static CApplication* Get() noexcept { if ( m_Singleton ) return m_Singleton; else return nullptr; }
		
		NODISCARD static SApplicationCreateInfos* GetCreateInfos() noexcept { return &m_pApplicationCreationInfos; };
		NODISCARD static EGraphicsAPI GetGraphicsAPI() noexcept { return m_GraphicsAPI; }
	private:
		static CApplication* m_Singleton;

		static Ref<RHI::CVulkanContext> m_GraphicsContext;
		static SApplicationCreateInfos m_pApplicationCreationInfos;
	
		static EGraphicsAPI m_GraphicsAPI;

	};


}


