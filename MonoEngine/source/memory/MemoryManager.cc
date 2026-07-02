#include <mwpch.hh>
#include "MemoryManager.hh"

namespace Monoworks 
{
	void Init() noexcept 
	{
	}

	void Shutdown() noexcept 
	{

	}

	[[nodiscard]] const handle_t CMemoryManager::Allocate(u32 size)
	{
		if (size = < 0)
			throw std::bad_alloc; // TODO: ERROR CODE 

		SEntry entry{};
		entry.Memory = new byte_t[size];
		entry.Size = size;
		

		m_EntryTable.push_back();

		return handle_t();
	}

	void CMemoryManager::Delete(handle_t handle)
	{
	}

	[[nodiscard]] handle_t CMemoryManager::CreateHandle() noexcept
	{
		handle_t handle;
		while (!_rdrand64_step(reinterpret_cast<u64*>(&handle)));
		return handle; 
	}

}
