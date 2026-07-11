#pragma once
#include <common/Base.hh>

#include <mutex>
#include <queue>
#include <atomic>
#include <concepts>

namespace Monoworks 
{
	namespace impl 
	{
		template <typename T>
		concept CanFront = requires(T t) { t.front(); };

		template <typename T>
		concept CanTop = requires(T t) { t.top(); };

		template <typename T>
		concept IsQueue = CanTop<T> || CanFront<T>;
	}

	template <typename T, class Q = std::queue<T>> requires impl::IsQueue<Q>
	class CSafeQueue 
	{
	public:
		[[nodiscard]] T Front() requires impl::CanFront<Q> noexcept
		{
			std::lock_guard<std::mutex> lock;
			--m_Size;
			T temp = std::move(m_Queue.front());
			m_Queue.pop();
			return temp;
		};

		[[nodiscard]] T Front() requires impl::CanTop<Q> noexcept
		{
			std::lock_guard<std::mutex> lock;
			--m_Size;
			T temp = std::move(m_Queue.top());
			m_Queue.pop();
			return temp;
		};

		void Push(T&& value) noexcept
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			++m_Size;
			m_Queue.push(std::forward<T>(value));
			m_ConditionVariable.notify_one();
		}

		[[nodiscard]] bool IsEmpty() const noexcept 
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			return m_Size == 0;
		}

		[[nodiscard]] u32 GetSize() const noexcept
		{
			return m_Size;
		}

	private:
		Q m_Queue;
		mutable std::mutex m_Mutex;		
		std::atomic<u32> m_Size;
	};
}
