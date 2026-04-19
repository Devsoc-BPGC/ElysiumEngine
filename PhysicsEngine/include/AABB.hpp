#include <stdbool.h>

/**
 * @brief Represents a 2D vector.
 */
typedef struct {
    float x;
    float y;
} Vector2;

/**
 * @brief Represents an axis-aligned bounding box.
 */
typedef struct {
    Vector2 min;
    Vector2 max;
} AABB;

/**
 * @brief Represents a circle.
 */
typedef struct {
    float centerX;
    float centerY;
    float radius;
} Circle;
