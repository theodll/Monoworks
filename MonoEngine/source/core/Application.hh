#pragma once
#include <common/Base.hh>

#include <string>

namespace Monoworks 
{

	struct SExtent2D 
	{
		int Height;
		int Width;
	};

	struct SApplicationCreateInfos 
	{	
		[[maybe_unused]] std::string Name = "MonoEngine";
		[[maybe_unused]] SExtent2D RenderableExtent = { 640, 480 };
		[[maybe_unused]] int ArgumentCount = 0;
		[[maybe_unused]] char** Arguments = nullptr;
	};


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

		[[nodiscard]] static CApplication* Get() noexcept { if (m_Singleton) return m_Singleton; else return nullptr;  }
	
	private:
		static CApplication* m_Singleton;

	};


}


