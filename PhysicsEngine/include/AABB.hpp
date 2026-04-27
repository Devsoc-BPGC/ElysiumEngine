#ifndef AABB_HPP
#define AABB_HPP

#include "CoreMath.hpp"

/**
 * @struct AABB
 * @brief Simple Axis-Aligned Bounding Box used for broad-phase collision detection.
 */
struct AABB {
    Vec3 min; /**< The minimum point of the box. */
    Vec3 max; /**< The maximum point of the box. */
};

#endif
