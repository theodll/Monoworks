#include <mwpch.hh>
#include <mutex>
#include "MemoryManager.hh"

namespace Monoworks
{
	std::vector<CMemoryManager::SEntry> CMemoryManager::s_EntryTable;
	std::vector<u32> CMemoryManager::s_FreeList;
	std::mutex CMemoryManager::s_Mutex;

	void CMemoryManager::Init() noexcept
	{
		s_EntryTable.reserve(1024);
		s_EntryTable.emplace_back();

		MW_INFO("Initialize CMemoryManger");

	}

	void CMemoryManager::Shutdown() noexcept
	{
		std::lock_guard<std::mutex> Lock(s_Mutex);
		for (auto& Entry : s_EntryTable)
		{
			if (Entry.Alive)
				::operator delete(Entry.pMemory);
		}
		s_EntryTable.clear();
		s_FreeList.clear();

		MW_INFO("Shutdown CMemoryManger");
	}

	[[nodiscard]] SHandle CMemoryManager::Allocate(u32 size) noexcept
	{
		if (size == 0)
			return SHandle{};

		void* pMemory = ::operator new(size, std::nothrow);
		if (!pMemory)
			return SHandle{};

		std::lock_guard<std::mutex> Lock(s_Mutex);

		u32 Index;
		if (!s_FreeList.empty())
		{
			Index = s_FreeList.back();
			s_FreeList.pop_back();
		}
		else
		{
			Index = (u32)s_EntryTable.size();
			s_EntryTable.emplace_back();
		}

		SEntry& Entry = s_EntryTable[Index];
		Entry.pMemory = pMemory;
		Entry.Size = size;
		Entry.Alive = true;

		MW_TRACE("Allocate {} bytes at {}", size, pMemory);

		return SHandle{ Index, Entry.Generation };
	}

	void CMemoryManager::Delete(const SHandle handle) noexcept
	{
		std::lock_guard<std::mutex> Lock(s_Mutex);

		if (handle.Index == 0 || handle.Index >= s_EntryTable.size())
			return;

		SEntry& Entry = s_EntryTable[handle.Index];
		if (!Entry.Alive || Entry.Generation != handle.Generation)
			return; 

		MW_TRACE("Delete {} bytes at handle index {} with address {}", Entry.Size, handle.Index, Entry.pMemory);

		::operator delete(Entry.pMemory);
		Entry.pMemory = nullptr;
		Entry.Alive = false;
		Entry.Generation++;


		s_FreeList.push_back(handle.Index);
	}

	[[nodiscard]] void* CMemoryManager::Get(const SHandle handle) noexcept
	{
		std::lock_guard<std::mutex> Lock(s_Mutex);

		if (handle.Index == 0 || handle.Index >= s_EntryTable.size())
			return nullptr;

		SEntry& Entry = s_EntryTable[handle.Index];
		if (!Entry.Alive || Entry.Generation != handle.Generation)
			return nullptr;

		return Entry.pMemory;
	}
    
	[[nodiscard]] bool CMemoryManager::IsValid(const SHandle handle) noexcept
    {
		if (handle.Index == 0)
			return false;

		uptr_t ptr = (uptr_t)CMemoryManager::Get(handle);
		if (ptr == 0)
			return false;
		
		if (!s_EntryTable[handle.Index].Alive)
			return false;
		
		if (!s_EntryTable[handle.Index].pMemory)
			return false;
		
        return true;
    }
}