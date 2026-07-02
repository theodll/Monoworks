#pragma once
#include <Common/Base.hh>

#include <vector>

namespace Monoworks 
{
	using handle_t = u64;

	struct SHeader 
	{
		handle_t Handle;
		u32		 Size;
		bool     Occupied;
	};

	struct SEntry 
	{
		handle_t Handle;
		byte_t* Memory;
		u32	 Size;
	};

	class CMemoryManager 
	{
	public: 
		CMemoryManager() = delete;

		static void Init() noexcept;
		static void Shutdown() noexcept;

		[[nodiscard]] static const handle_t Allocate(u32 size);
		static void Delete(const handle_t handle);

	private:
		[[nodiscard]] handle_t CreateHandle() noexcept;


		static std::vector<SEntry> m_EntryTable;
		static u32 m_HandleCount;
	};

}