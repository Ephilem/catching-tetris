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

#endif
