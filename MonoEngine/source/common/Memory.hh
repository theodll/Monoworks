/**
 * @file Memory.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Reference counting system for automatic memory management
 * @version 0.1
 * @date 2026-07-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once
#include <memory/MemoryManager.hh>
#include <common/Base.hh>

#include <atomic>

namespace Monoworks 
{
	/**
	 * @brief Base class to inherit by classes that are reference counted
	 */
	class RefCounted 
	{
	public:
		virtual ~RefCounted() = default;

		void IncRefCount() noexcept
		{
			++m_ReferenceCount;
		}

		void DecRefCount() noexcept
		{
			--m_ReferenceCount;
		}
		
		/**
		 * @brief Get the Ref Count object
		 * @return const u32 
		 * Immutable Reference Count returned
		 */
		[[nodiscard]] const u32 GetRefCount() const noexcept { return m_ReferenceCount.load(); };
	private:
		mutable std::atomic<u32> m_ReferenceCount;
	};

	/**
	 * @brief Shared Pointer
	 * 
	 * @tparam T Type of the object of the memory
	 */
	template <typename T>
	class Ref 
	{
	public:
		Ref() : m_Instance({}) const noexcept {};
		Ref(std::nullptr_t n) : m_Instance({}) const noexcept {};
		
		Ref(const SHandle instance)	noexcept : m_Instance(instance) 
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		Ref(const Ref<T2>& other) noexcept
		{
			m_Instance = other.m_Instance;
			IncRef();
		}

		template<typename T2>
		Ref(Ref<T2>&& other) noexcept
		{
			m_Instance = other.m_Instance;
			other.m_Instance = MW_NULL_MEMORY;
		}

		[[nodiscard]] static Ref<T> CopyWithoutIncrement(const Ref<T>& other) noexcept
		{
			Ref<T> result = nullptr;
			result->m_Instance = other.m_Instance;
			return result;
		}

		~Ref() noexcept
		{
			DecRef();
		}

		Ref(const Ref<T>& other) noexcept
			: m_Instance(other.m_Instance)
		{
			IncRef();
		}

		Ref& operator=(std::nullptr_t) noexcept
		{
			DecRef();
			m_Instance = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other) noexcept
		{
			if (this == &other)
				return *this;

			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		/**
		 * @brief Creates a shared pointer (like std::make_shared)
		 * @return Ref<T> Returns the created shared pointer
		 */
		[[nodiscard]] static Ref<T> Create() noexcept
		{
			SHandle handle;

			handle = CMemoryManager::Allocate(sizeof(T)); 
			
			return Ref<T>(handle);
			
		};

		/**
		 * @brief Accesses the raw underlying pointer if its valid.
		 * @return const T* Raw immutable pointer to the object
		 */
		[[nodiscard]] const T const * raw() const noexcept 
		{ 
			if (CMemoryManager::IsValid) 
			{
				 return static_cast<T*>(CMemoryManager::Get(m_Instance)); 
			} else 
			{
				throw std::runtime_error
			}


		};
		[[nodiscard]] const T const * operator->() const noexcept { return raw(); };

	private:
		SHandle m_Instance;
	};
}