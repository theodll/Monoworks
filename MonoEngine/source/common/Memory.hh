#pragma once
#include <memory/MemoryManager.hh>
#include <common/Base.hh>

#include <atomic>

namespace Monoworks 
{
	class RefCounted 
	{
	public:
		virtual ~RefCounted() = default;

		void IncRefCount() const noexcept
		{
			++m_ReferenceCount;
		}

		void DecRefCount() const noexcept
		{
			--m_ReferenceCount;
		}
		
		[[nodiscard]] const u32 GetRefCount() const noexcept { return m_RefCount.load(); };
	private:
		mutable std::atomic<u32> m_ReferenceCount;
	};

	template <typename T>
	class Ref 
	{
	public:
		Ref() : m_Instance({}) const noexcept {};
		Ref(std::nullptr_t n) : m_Instance({}) const noexcept {};
		Ref(const SHandle instance) : m_Instance(instance) const noexcept 
		{
			INCREF
		}


		[[nodiscard]] static Ref<T> Create() noexcept;

		[[nodiscard]] const T* raw() const noexcept { static_cast<T*>(CMemoryManager::Get(m_Instance)); };
		[[nodiscard]] const T* operator->() const noexcept { return raw(); };
		


	private:
		SHandle m_Instance;
	};
}