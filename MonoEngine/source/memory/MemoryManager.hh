/**
 * @file MemoryManager.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief System to manage raw memory and defragment memory
 * @version 0.1
 * @date 2026-07-03
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <common/Base.hh>
#include <vector>
#include <mutex>

namespace Monoworks
{
	struct SHandle
	{
		u32 Index = 0;
		u32 Generation = 0;
	};

	static constexpr SHandle MW_NULL_MEMORY = { .Index{0}, .Generation{0} };

	class CMemoryManager
	{
	public:
		CMemoryManager() = delete;

		/**
		 * @brief Initialises the Memory Manager
		 */
		static void Init() noexcept;

		/**
		 * @brief Shuts down the Memory Manager
		 */
		static void Shutdown() noexcept;

		/**
		 * @brief Allocates the required memory and returns the corresponding SHandle
		 * @param size
		 * Required size in bytes of memory to be allocated
		 * @return SHandle
		 * SHandle corresponding to the allocated memory
		 */
		[[nodiscard]] static SHandle Allocate(u32 size) noexcept;

		/**
		 * @brief Deletes the memory corresponding to the given handle
		 * @param handle SHandle corresponding to the allocated memory
		 */
		static void Delete(const SHandle handle) noexcept;

		/**
		 * @brief Accesses the memory of the corresponding SHandle
		 * @param handle
		 * SHandle corresponding to the memory to access
		 * @return void*
		 * Pointer to the allocated memory
		 */
		[[nodiscard]] static void* Get(const SHandle handle) noexcept;

		/**
		 * @brief Checks if the handle is a valid handle.
		 * @param handle SHandle to be checked
		 * @return true SHandle and underlying memory is valid and ready to be accessed
		 * @return false SHandle or underlying memory is invalid or is being relocated
		 */
		[[nodiscard]] static bool IsValid(const SHandle handle) noexcept;


	private:
		struct SEntry
		{
			void* pMemory = nullptr;
			u32   Size = 0;
			u32   Generation = 0;
			bool  Alive = false;
		};

		static std::vector<SEntry> s_EntryTable;
		static std::vector<u32>    s_FreeList;
		static std::mutex          s_Mutex;
	};
}