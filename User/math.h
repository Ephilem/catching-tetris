#ifndef MATH_H
#define MATH_H
#include <stdlib.h>

typedef struct {
    int16_t x;
    int16_t y;
} ivec2;

typedef struct {
    ivec2 min;
    ivec2 max;
} aabb;

static inline int rand_range(int min, int max) {
    return min + (rand() % (max - min + 1));
}

static inline uint8_t aabb_is_colliding(aabb a, aabb b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y);
}

#endif
