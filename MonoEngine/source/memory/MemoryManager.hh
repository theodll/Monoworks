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

	class CMemoryManager
	{
	public:
		CMemoryManager() = delete;

		static void Init() noexcept;
		static void Shutdown() noexcept;

		[[nodiscard]] static SHandle Allocate(u32 size) noexcept;
		static void Delete(const SHandle handle) noexcept;
		[[nodiscard]] static void* Get(const SHandle handle) noexcept;

	private:
		struct SEntry
		{
			void* Memory = nullptr;
			u32   Size = 0;
			u32   Generation = 0;
			bool  Alive = false;
		};

		static std::vector<SEntry> s_EntryTable;
		static std::vector<u32>    s_FreeList;
		static std::mutex          s_Mutex;
	};
}