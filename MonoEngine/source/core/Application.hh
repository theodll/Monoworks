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
		[[maybe_unused]] const char* Name = "MonoEngine";
		/**
		* @brief Array of arguments.
		*/
		[[maybe_unused]] char** Arguments = nullptr;
		/**
		 * @brief Callback to retrieve
		 */
		[[maybe_unused]] const char** (*RequiredExtensionCallback)(u32* extensionCount);
		/**
		 * @brief Extent the engine is able to render to (eg. Window/Viewport size)
		 */
		[[maybe_unused]] SExtent2D RenderableExtent = { 640, 480 };
		/**
		 * @brief Graphics API used by the renderer.
		 */
		[[maybe_unused]] EGraphicsAPI GraphicsAPI;
		/**
		* @brief Number of items in the Arguments array.
		*/
		[[maybe_unused]] s32 ArgumentCount = 0;
		/**
		 * @brief Version of the associated Application.
		 */
		[[maybe_unused]] SAppVersion Version;	
		/**
		 * @brief Define whether to use the Vulkan swapchain.
		 */
		[[maybe_unused]] bool UseSwapchain;
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
		void Init(const SApplicationCreateInfos* pInfos) noexcept;

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
		[[nodiscard]] static CApplication* Get() noexcept { if (m_Singleton) return m_Singleton; else return nullptr;  }
	
		[[nodiscard]] static SApplicationCreateInfos* GetCreateInfos() noexcept { return &m_pApplicationCreationInfos; };
		[[nodiscard]] static EGraphicsAPI* GetGraphicsAPI() noexcept { return &m_GraphicsAPI; }
	private:
		static CApplication* m_Singleton;

		static Ref<RHI::CVulkanContext> m_GraphicsContext;
		static SApplicationCreateInfos m_pApplicationCreationInfos;
	
		static EGraphicsAPI m_GraphicsAPI;

	};


}


