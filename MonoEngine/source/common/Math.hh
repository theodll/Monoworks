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

#define GLM_ENABLE_EXPERIMENTAL
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