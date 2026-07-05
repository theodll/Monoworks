/**
 * @file Memory.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Reference counting system for automatic memory management
 * @version 0.2
 * @date 2026-07-05
 *
 * @copyright Copyright (c) 2026
 */

#pragma once
#include <memory/MemoryManager.hh>
#include <common/Base.hh>

#include <atomic>
#include <utility>
#include <type_traits>
#include <new> 

namespace Monoworks
{
	inline bool operator==(const SHandle& a, const SHandle& b) noexcept {
		return a.Index == b.Index && a.Generation == b.Generation;
	}
	inline bool operator!=(const SHandle& a, const SHandle& b) noexcept {
		return !(a == b);
	}

	/**
	 * @brief Shared Pointer wrapper for custom SHandle memory allocations.
	 *
	 * @tparam T Type of the object in memory
	 */
	template <typename T>
	class Ref
	{
	private:

		struct ControlBlock
		{
			std::atomic<u32> RefCount;
			T Object;

			template <typename... Args>
			ControlBlock(Args&&... args)
				: RefCount(1), Object(std::forward<Args>(args)...) {
			}
		};

		SHandle m_Handle = MW_NULL_MEMORY;

		[[nodiscard]] ControlBlock* GetControlBlock() const noexcept
		{
			if (m_Handle != MW_NULL_MEMORY && CMemoryManager::IsValid(m_Handle))
			{
				return static_cast<ControlBlock*>(CMemoryManager::Get(m_Handle));
			}
			return nullptr;
		}

		void IncRef() noexcept
		{
			if (ControlBlock* cb = GetControlBlock())
			{
				cb->RefCount.fetch_add(1, std::memory_order_relaxed);
			}
		}

		void DecRef() noexcept
		{
			ControlBlock* cb = GetControlBlock();
			if (cb)
			{
				if (cb->RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					cb->~ControlBlock();
					CMemoryManager::Delete(m_Handle);
				}
			}
			m_Handle = MW_NULL_MEMORY;
		}

	public:
		Ref() noexcept = default;
		Ref(std::nullptr_t) noexcept {}

		Ref(const Ref<T>& other) noexcept
			: m_Handle(other.m_Handle)
		{
			IncRef();
		}

		Ref(Ref<T>&& other) noexcept
			: m_Handle(other.m_Handle)
		{
			other.m_Handle = MW_NULL_MEMORY;
		}

		~Ref() noexcept
		{
			DecRef();
		}

		Ref& operator=(const Ref<T>& other) noexcept
		{
			if (this != &other)
			{
				DecRef();
				m_Handle = other.m_Handle;
				IncRef();
			}
			return *this;
		}

		Ref& operator=(Ref<T>&& other) noexcept
		{
			if (this != &other)
			{
				DecRef();
				m_Handle = other.m_Handle;
				other.m_Handle = MW_NULL_MEMORY;
			}
			return *this;
		}

		Ref& operator=(std::nullptr_t) noexcept
		{
			DecRef();
			return *this;
		}

		/**
		 * @brief Creates a shared pointer (like std::make_shared)
		 * @param args Arguments to forward to T's constructor
		 * @return Ref<T> Returns the created shared pointer
		 */
		template <typename... Args>
		[[nodiscard]] static Ref<T> Create(Args&&... args)
		{
			SHandle handle = CMemoryManager::Allocate(sizeof(ControlBlock));
			void* rawMem = CMemoryManager::Get(handle);

			new (rawMem) ControlBlock(std::forward<Args>(args)...);

			Ref<T> ref;
			ref.m_Handle = handle;
			return ref;
		};

		/**
		 * @brief Accesses the underlying pointer
		 * @return T const * Returns the underlying pointer as immutable
		*/
		[[nodiscard]] T const* raw() const noexcept
		{
			if (!CMemoryManager::IsValid(m_Handle))
				return nullptr;

			if (ControlBlock* cb = GetControlBlock())
			{
				return &cb->Object;
			}
			return nullptr;
		}

		[[nodiscard]] T const* operator->() const noexcept
		{
			return raw();
		}

		[[nodiscard]] T& operator*() const noexcept
		{
			return *raw();
		}

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return GetControlBlock() != nullptr && CMemoryManager::IsValid(m_Handle);
		}

		[[nodiscard]] const SHandle& GetHandle() const noexcept
		{
			if (CMemoryManager::IsValid(m_Handle))
				return m_Handle;
			return MW_NULL_MEMORY;
		}
	};

	template<class T, class U>
	inline bool operator==(const Ref<T>& a, const Ref<U>& b) noexcept
	{
		return a.GetHandle() == b.GetHandle();
	}

	template<class T, class U>
	inline bool operator!=(const Ref<T>& a, const Ref<U>& b) noexcept
	{
		return !(a == b);
	}
}