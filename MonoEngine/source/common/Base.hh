/**
 * @defgroup Common Common
 * @brief Common Components shared between every Application
 */
 

/**
 * @file Base.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Common definitions for the entire engine
 * @version 0.1
 * @date 2026-07-03
 * @ingroup Common
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once
#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

using uptr_t = uintptr_t;
using byte_t = unsigned char;

namespace Monoworks
{
	enum EGraphicsAPI
	{
		MW_GAPI_NONE = 0x0, 
		MW_GAPI_VULKAN = 0x10000000
	};

	/**
	 * @brief Two-Dimensional Extent Structure
	 */
	struct SExtent2D
	{
		/**
		 * @brief Height component of the Two-Dimensional Extent.
		 */
		u32 Height = 0;

		/**
		 * @brief Width component of the Two-Dimensional Extent.
		 */
		u32 Width = 0;
	};

}
