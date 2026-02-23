#include <stdbool.h>

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    Vector2 min;
    Vector2 max;
} AABB;

typedef struct {
    float centerX;
    float centerY;
    float radius;
} Circle;
