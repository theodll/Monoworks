/**
 * @file Memory.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Reference counting system for automatic memory management
 * @version 0.3
 * @date 2026-07-20
 * @ingroup Common
 * @copyright Copyright (c) 2026
 */

#pragma once
#include <core/MemoryManager.hh>
#include <common/Base.hh>
#include "Log.hh"

#include <atomic>
#include <utility>
#include <type_traits>
#include <new>

namespace Monoworks
{
	inline bool operator==(const SHandle& a, const SHandle& b) NOEXCEPT {
		return a.Index == b.Index && a.Generation == b.Generation;
	}
	inline bool operator!=(const SHandle& a, const SHandle& b) NOEXCEPT {
		return !(a == b);
	}

	/**
	 * @brief Type-erased base of SControlBlock<U>. Owns the refcount and, via its
	 * virtual destructor, the correct destruction of whatever concrete U was
	 * allocated - independent of what CRef<T> is currently viewing it as.
	 */
	struct SControlBlockBase
	{
		std::atomic<u32> RefCount;

		SControlBlockBase() NOEXCEPT : RefCount(1) {}

		virtual ~SControlBlockBase() NOEXCEPT = default;
	};

	/**
	 * @brief Concrete control block holding the object of type U.
	 * Allocated via CMemoryManager as SControlBlock<U>, but only ever referenced
	 * afterwards through the type-erased SControlBlockBase* for lifetime handling.
	 *
	 * @tparam U Concrete type of the object in memory
	 */
	template <typename U>
	struct SControlBlock : SControlBlockBase
	{
		U Object;

		template <typename... Args>
		SControlBlock(Args&&... args)
			: SControlBlockBase()
			, Object(std::forward<Args>(args)...)
		{
		}
	};

	/**
	 * @brief Shared Pointer wrapper for custom SHandle memory allocations.
	 * Supports upcasting: CRef<T> can be constructed/assigned from a CRef<U>
	 * whenever U* implicitly converts to T*, e.g. CRef<IBase> = CRef<CDerived>::Create( ... );
	 *
	 * @tparam T Type of the object in memory
	 */
	template <typename T>
	class CRef
	{
		template <typename>
		friend class CRef;

	private:

		SHandle m_Handle = MW_NULL_MEMORY;
		T* m_pPtr = nullptr;

		NODISCARD SControlBlockBase* GetControlBlock() const NOEXCEPT
		{
			MW_PROFILE_FUNC;
			if (m_Handle != MW_NULL_MEMORY && CMemoryManager::IsValid(m_Handle))
			{
				return static_cast<SControlBlockBase*>(CMemoryManager::Get(m_Handle));
			}
			return nullptr;
		}

		void IncRef() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			if (SControlBlockBase* pBlock = GetControlBlock())
			{
				pBlock->RefCount.fetch_add(1, std::memory_order_relaxed);
			}
		}

		void DecRef() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			SControlBlockBase* pBlock = GetControlBlock();
			if (pBlock)
			{
				if (pBlock->RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					pBlock->~SControlBlockBase();
					CMemoryManager::Delete(m_Handle);
				}
			}
			m_Handle = MW_NULL_MEMORY;
			m_pPtr = nullptr;
		}

	public:
		CRef() NOEXCEPT = default;
		CRef(std::nullptr_t) NOEXCEPT {}

		CRef(const CRef<T>& other) NOEXCEPT
			: m_Handle(other.m_Handle)
			, m_pPtr(other.m_pPtr)
		{
			MW_PROFILE_FUNC;
			IncRef();
		}

		CRef(CRef<T>&& other) NOEXCEPT
			: m_Handle(other.m_Handle)
			, m_pPtr(other.m_pPtr)
		{
			MW_PROFILE_FUNC;
			other.m_Handle = MW_NULL_MEMORY;
			other.m_pPtr = nullptr;
		}

		/**
		 * @brief Converting copy constructor, enabled when U* converts to T* (upcast).
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		CRef(const CRef<U>& other) NOEXCEPT
			: m_Handle(other.m_Handle)
			, m_pPtr(other.m_pPtr)
		{
			MW_PROFILE_FUNC;
			IncRef();
		}

		/**
		 * @brief Converting move constructor, enabled when U* converts to T* (upcast).
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		CRef(CRef<U>&& other) NOEXCEPT
			: m_Handle(other.m_Handle)
			, m_pPtr(other.m_pPtr)
		{
			MW_PROFILE_FUNC;
			other.m_Handle = MW_NULL_MEMORY;
			other.m_pPtr = nullptr;
		}

		~CRef() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			DecRef();
		}

		CRef& operator=(const CRef<T>& other) NOEXCEPT
		{
			if (this != &other)
			{
				DecRef();
				m_Handle = other.m_Handle;
				m_pPtr = other.m_pPtr;
				IncRef();
			}
			return *this;
		}

		CRef& operator=(CRef<T>&& other) NOEXCEPT
		{
			if (this != &other)
			{
				DecRef();
				m_Handle = other.m_Handle;
				m_pPtr = other.m_pPtr;
				other.m_Handle = MW_NULL_MEMORY;
				other.m_pPtr = nullptr;
			}
			return *this;
		}


		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		CRef& operator=(const CRef<U>& other) NOEXCEPT
		{
			DecRef();
			m_Handle = other.m_Handle;
			m_pPtr = other.m_pPtr;
			IncRef();
			return *this;
		}

		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		CRef& operator=(CRef<U>&& other) NOEXCEPT
		{
			DecRef();
			m_Handle = other.m_Handle;
			m_pPtr = other.m_pPtr;
			other.m_Handle = MW_NULL_MEMORY;
			other.m_pPtr = nullptr;
			return *this;
		}

		CRef& operator=(std::nullptr_t) NOEXCEPT
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
		NODISCARD static CRef<T> Create(Args&&... args)
		{
			MW_PROFILE_FUNC;
			using SBlock = SControlBlock<T>;

			SHandle handle = CMemoryManager::Allocate(static_cast<u32>(sizeof(SBlock)));
			void* pRawMem = CMemoryManager::Get(handle);

			if ( !pRawMem )
			{
				return CRef<T>();
			}

			SBlock* pBlock = new ( pRawMem ) SBlock(std::forward<Args>(args)...);

			CRef<T> ref;
			ref.m_Handle = handle;
			ref.m_pPtr = &pBlock->Object;
			return ref;
		};

		/**
		 * @brief Frees the reference.
		 */
		void Reset() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			DecRef();
		}
		
		/**
		 * @brief Replaces the current CRef with other.
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		void Swap( const CRef<U>& other ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			*this = other;
		}
		
		
		/**
	    * @brief Replaces the current CRef with other.
	    * Same as CRef::Swap
	    */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		void Reset( const CRef<U>& other ) NOEXCEPT
		{
			MW_PROFILE_FUNC;
			Swap( other );
		}


		/**
		* @brief dynamic downcast
		*/
		template <typename U>
		NODISCARD CRef<U> As() const NOEXCEPT
		{
			MW_PROFILE_FUNC;

			if ( !m_pPtr || !CMemoryManager::IsValid( m_Handle ) ) UNLIKELY
			{
				return CRef<U>();
			}

			U* pCastPtr = dynamic_cast< U* >( m_pPtr );
			if ( !pCastPtr || !CMemoryManager::IsValid( m_Handle ) ) UNLIKELY
			{
				return CRef<U>();
			}

			if ( !CMemoryManager::IsValid( m_Handle ) ) UNLIKELY
			{
				return CRef<U>();
			}

			CRef<U> result;
			result.m_Handle = m_Handle;
			result.m_pPtr = pCastPtr;
			result.IncRef(); 

			return result;
		}

		/**
		 * @brief Accesses the underlying pointer
		 * @return T const * Returns the underlying pointer as immutable
		*/
		NODISCARD T const* raw() const NOEXCEPT
		{
			if (!CMemoryManager::IsValid(m_Handle))
				return nullptr;
			return m_pPtr;
		}

		/**
		 * @brief Accesses the underlying pointer
		 * @return T const * Returns the underlying pointer as mutable
		*/
		NODISCARD T* raw() NOEXCEPT
		{
			if (!CMemoryManager::IsValid(m_Handle))
				return nullptr;
			return m_pPtr;
		}

		NODISCARD T& operator*() NOEXCEPT
		{
			return *raw();
		}

		NODISCARD T* operator->() NOEXCEPT
		{
			return raw();
		}

		NODISCARD T const* operator->() const NOEXCEPT
		{
			return raw();
		}

		NODISCARD T& operator*() const NOEXCEPT
		{
			return *raw();
		}

		NODISCARD explicit operator bool() const NOEXCEPT
		{
			return m_pPtr != nullptr && CMemoryManager::IsValid(m_Handle);
		}

		NODISCARD const SHandle& GetHandle() const NOEXCEPT
		{
			MW_PROFILE_FUNC;
			if (CMemoryManager::IsValid(m_Handle))
				return m_Handle;
			return MW_NULL_MEMORY;
		}
	};

	template <class T, class U>
	inline bool operator==(const CRef<T>& a, const CRef<U>& b) NOEXCEPT
	{
		return a.GetHandle() == b.GetHandle();
	}

	template <class T, class U>
	inline bool operator!=(const CRef<T>& a, const CRef<U>& b) NOEXCEPT
	{
		return !(a == b);
	}

	template <typename T>
	class CUnique
	{
		template <typename>
		friend class CUnique;

	private:

		using FDestroy = void ( * )( void* );

		SHandle m_Handle = MW_NULL_MEMORY;
		T* m_pPtr = nullptr;
		FDestroy m_pDestroy = nullptr;

		void Destroy() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			if ( m_pDestroy && CMemoryManager::IsValid( m_Handle ) )
			{
				m_pDestroy( CMemoryManager::Get( m_Handle ) );
				CMemoryManager::Delete( m_Handle );
			}
			m_Handle = MW_NULL_MEMORY;
			m_pPtr = nullptr;
			m_pDestroy = nullptr;
		}

	public:
		CUnique() NOEXCEPT = default;
		CUnique( std::nullptr_t ) NOEXCEPT {}

		CUnique( const CUnique& ) = delete;
		CUnique& operator=( const CUnique& ) = delete;

		CUnique( CUnique&& other ) NOEXCEPT
			: m_Handle( other.m_Handle )
			, m_pPtr( other.m_pPtr )
			, m_pDestroy( other.m_pDestroy )
		{
			other.m_Handle = MW_NULL_MEMORY;
			other.m_pPtr = nullptr;
			other.m_pDestroy = nullptr;
		}

		/**
		 * @brief Converting move constructor, enabled when U* converts to T* (upcast).
		 */
		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		CUnique( CUnique<U>&& other ) NOEXCEPT
			: m_Handle( other.m_Handle )
			, m_pPtr( other.m_pPtr )
			, m_pDestroy( other.m_pDestroy )
		{
			other.m_Handle = MW_NULL_MEMORY;
			other.m_pPtr = nullptr;
			other.m_pDestroy = nullptr;
		}

		~CUnique() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			Destroy();
		}

		CUnique& operator=( CUnique&& other ) NOEXCEPT
		{
			if ( this != &other )
			{
				Destroy();
				m_Handle = other.m_Handle;
				m_pPtr = other.m_pPtr;
				m_pDestroy = other.m_pDestroy;
				other.m_Handle = MW_NULL_MEMORY;
				other.m_pPtr = nullptr;
				other.m_pDestroy = nullptr;
			}
			return *this;
		}

		template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		CUnique& operator=( CUnique<U>&& other ) NOEXCEPT
		{
			Destroy();
			m_Handle = other.m_Handle;
			m_pPtr = other.m_pPtr;
			m_pDestroy = other.m_pDestroy;
			other.m_Handle = MW_NULL_MEMORY;
			other.m_pPtr = nullptr;
			other.m_pDestroy = nullptr;
			return *this;
		}

		CUnique& operator=( std::nullptr_t ) NOEXCEPT
		{
			Destroy();
			return *this;
		}

		/**
		 * @brief Creates a unique pointer (like std::make_unique).
		 * Concrete type selectable: CUnique<IBase>::Create<CDerived>( ... )
		 * @tparam U Concrete type to construct, defaults to T
		 * @param args Arguments to forward to U's constructor
		 */
		template <typename U = T, typename... Args>
		NODISCARD static CUnique<T> Create( Args&&... args )
		{
			MW_PROFILE_FUNC;
			static_assert( std::is_convertible_v<U*, T*>, "U* must be convertible to T*" );

			SHandle handle = CMemoryManager::Allocate( static_cast< u32 >( sizeof( U ) ) );
			void* pRawMem = CMemoryManager::Get( handle );

			if ( !pRawMem ) UNLIKELY
			{
				return CUnique<T>();
			}

			U* pObject = new ( pRawMem ) U( std::forward<Args>( args )... );

			CUnique<T> result;
			result.m_Handle = handle;
			result.m_pPtr = pObject;
			result.m_pDestroy = []( void* pMem ) { static_cast< U* >( pMem )->~U(); };
			return result;
		}

		/**
		 * @brief Destroys the owned object.
		 */
		void Reset() NOEXCEPT
		{
			MW_PROFILE_FUNC;
			Destroy();
		}

		/**
		 * @brief Exchanges ownership with other (std::swap-Semantik).
		 */
		void Swap( CUnique& other ) NOEXCEPT
		{
			std::swap( m_Handle, other.m_Handle );
			std::swap( m_pPtr, other.m_pPtr );
			std::swap( m_pDestroy, other.m_pDestroy );
		}

		NODISCARD T* raw() const NOEXCEPT { return m_pPtr; }
		NODISCARD T& operator*() const NOEXCEPT { return *m_pPtr; }
		NODISCARD T* operator->() const NOEXCEPT { return m_pPtr; }
		NODISCARD explicit operator bool() const NOEXCEPT { return m_pPtr != nullptr; }
		NODISCARD const SHandle& GetHandle() const NOEXCEPT { return m_Handle; }
	};

	template <class T, class U>
	inline bool operator==( const CUnique<T>& a, const CUnique<U>& b ) NOEXCEPT
	{
		return a.GetHandle() == b.GetHandle();
	}

	template <class T, class U>
	inline bool operator!=( const CUnique<T>& a, const CUnique<U>& b ) NOEXCEPT
	{
		return !( a == b );
	}
}

template <typename T>
using Ref = Monoworks::CRef<T>;

template <typename T>
using Unique = Monoworks::CUnique<T>;

