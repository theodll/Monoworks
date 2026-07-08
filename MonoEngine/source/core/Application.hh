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

#include <string>

namespace Monoworks 
{

	/**
	 * @brief Two-Dimensional Extent Structure
	 */
	struct SExtent2D 
	{
		/**
		 * @brief Height component of the Two-Dimensional Extent.
		 */
		int Height;

		/**
		 * @brief Width component of the Two-Dimensional Extent.
		 */
		int Width;
	};

	/**
	 * @brief All infos the Engine needs to initialize
	 */
	struct SApplicationCreateInfos 
	{	
		/**
		 * @brief Name of the Application the engine is integrated in.
		 */
		[[maybe_unused]] std::string Name = "MonoEngine";

		/**
		 * @brief Extent the engine is able to render to (eg. Window/Viewport size)
		 */
		[[maybe_unused]] SExtent2D RenderableExtent = { 640, 480 };

		/**
		 * @brief Number of items in the Arguments array.
		 */
		[[maybe_unused]] int ArgumentCount = 0;

		/**
		 * @brief Array of arguments.
		 */
		[[maybe_unused]] char** Arguments = nullptr;
	};

	/**
	 * @brief Root Engine Class managing the entire engine
	 */
	class CApplication 
	{
	public:
		CApplication() noexcept = default;
		virtual ~CApplication() noexcept = default;
		
		/**
		 * @brief Initializes the engine.
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
	
	private:
		static CApplication* m_Singleton;

	};


}


