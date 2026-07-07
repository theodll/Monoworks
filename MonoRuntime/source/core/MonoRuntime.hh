#include <Monoworks.hh>

namespace Monoworks 
{
	/**
	 * @brief Generic main function of the runtime.
	 * For example: main, WinMain, etc.
	 * @param argc Number of arguments in the array of arguments
	 * @param argv Array of arguments
	 * @return int Return code.
	 */
	[[nodiscard]] int RuntimeMain(int argc, char** argv);

	/**
	 * @brief Class to house the MonoRuntime
	 */
	class CMonoRuntime 
	{
	public:
		CMonoRuntime() = default;

		/**
		 * @brief Initializes the runtime 
		 * @param argc Number of arguments in the array of arguments
		 * @param argv Array of arguments
		 */
		void Init(int argc, char** argv);

		/**
		 * @brief Main Loop of the runtime
		 */
		void Run();

		/**
		 * @brief Gently shuts down the runtime 
		 */
		void Shutdown();
	
	private:
		CApplication* m_Application;

		bool m_Running;
	};

}
