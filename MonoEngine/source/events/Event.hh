/**
 * @file Event.hh
 * @author Theo Wimber (theowimber@abeams.app) 
 * @brief Houses all Event Structures
 * @version 0.1
 * @date 2026-07-11
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once
#include <common/Base.hh>

#include <concepts>

namespace Monoworks 
{
	/**
	 * @brief All constraints an Event type has to meet to be classified and available as Event
	 * @tparam T The type of the Event
	 */
	template <typename T>
	concept ValidEvent = std::is_trivially_copyable_v<T> && !std::is_pointer_v<T> && (sizeof(T) <= 60);

	/**
	 * @brief Type of the Event that a SEvent represents.
	 * This type allows a maximum of 256 different types of Events to take place. This enum is always bitshiftet into the first byte (or rather last byte) of the SEvent struct and is required to ensure safe casting and dispatching of these events.
	 */
	enum EEventType : u8
	{
		MW_EVENT_TYPE_MOUSE_CLICKED, 
		
		MW_EVENT_TYPE_COUNT
	};

	/**
	 * @brief Structure to house and dispatch any event with a payload smaller than 60 bytes
	 * 
	 * This structure is excactly 64 Bytes large and 64 Byte aligned. Thus it perfectly fits onto most platform's cache lines and significantly improves performance.
	 * Payload is a simple array of bytes to house basically any event and improve performance by not having to deal with runtime polymorphism but rather adding zero overhead while
	 * still being memory safe through modern template metaprogramming
	 */
	struct alignas(64) SEvent
	{
		/**
		 * @brief Simple header of the SEvent struct to house the type and any other light metadata a generic event might need 
		 */
		u32		Header;

		/**
		 * @brief Byte array to house data of basically any event by not having to deal with runtime polymorphism and eliminating the probabillity of cache misses
		 */
		byte_t	Payload[60];

		/**
		 * @brief Moves the entire payload of T into the Payload variable
		 * 
		 * @tparam T Type of the Event
		 * @param data The Event
		 */
		template <typename T> requires ValidEvent<T>
		void SetPayload(const T& data) noexcept
		{
			std::memcpy(Payload, &data, sizeof(T));
		}

		/**
		 * @brief Get the Payload object
		 * 
		 * @tparam T Type of the eventy
		 * @return T Copy of the data in the Payload object casted into T
		 */
		template <typename T> requires ValidEvent<T>
		[[nodiscard]] T GetPayload() const noexcept
		{
			T temp;
			std::memcpy(&temp, Payload, sizeof(T));
			return temp;
		}

		/**
		 * @brief Shifts the value of the type enum into the first byte of the Header
		 * @param type Type of the Event
		 */
		void SetType (EEventType type) noexcept
		{
			// little endian 
			Header = (Header & 0x00FFFFFF) | ((u8)type << 24);
		}

		/**
		 * @brief Get the Type 
		 * @return EEventType Type of the event
		 */
		[[nodiscard]] EEventType GetType() const noexcept 
		{
			return (EEventType)(u8)((Header >> 24) & 0xFF);
		}

	private:

		/**
		* @brief Shifts a boolean (represented as 1 byte unsigned integer) into the header at byte 2
		* @param handled
		*/
		void SetHandled(u8 handled) noexcept
		{
			// little endian
			Header = (Header & 0xFF00FFFF) | (handled << 16);
		}


		/**
		* @brief Get the Handled
		* @return bool Handle bool
		*/
		[[nodiscard]] bool GetHandled() const noexcept
		{
			return (bool)((Header >> 16) & 0xFF);
		}

		friend class CEventManager;
	};

	struct SClickedEvent
	{
		int x = 0;
		int y = 0;
	};







}
