#ifndef MATH_H
#define MATH_H

typedef struct {
    int16_t x;
    int16_t y;
} ivec2;

typedef struct {
    ivec2 min;
    ivec2 max;
} aabb;

#endif
