#pragma once
/**
 * @file Math.hh
 * @author Theo Wimber (theowimber@abeams.app)
 * @brief Defines aliases of glm types
 * @version 0.1
 * @date 2026-07-01
 * @ingroup Common
 * @copyright Copyright (c) 2026
 */

#include <glm/glm.hpp>

// For SIMD 
#ifndef GLM_FORCE_INTRINSICS
#define GLM_FORCE_INTRINSICS
#endif

#ifndef GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif

#if (_MSVC_VER && !__INTEL_COMPILER) 
#ifdef (__AVX2__)
#define MW_AVX2_SUPPORT
#endif
#elseif (__clang__)
#ifdef(__AVX2__)
#define MW_AVX2_SUPPORT
#endif 
#elseif (__INTEL_COMPILER)
#ifdef (__AVX2__)
#define MW_AVX2_SUPPORT
#endif
#elseif (__GNUC__)
#ifdef (__AVX2__)
#define MW_AVX2_SUPPORT
#endif
#endif


#ifdef MW_AVX2_SUPPORT
#ifndef GLM_FORCE_AVX2
#define GLM_FORCE_AVX2
#endif
#endif

// For Quaternions
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glm/gtx/quaternion.hpp>

namespace Monoworks 
{
    using Vector     = ::glm::vec3;
    using Vector2    = ::glm::vec2;
    using Vector4    = ::glm::vec4;

    using Matrix     = ::glm::mat4;
    using Matrix3    = ::glm::mat3;
    using Matrix2    = ::glm::mat2;

    using Quaternion = ::glm::quat;
    using Angle      = ::glm::vec3;
}