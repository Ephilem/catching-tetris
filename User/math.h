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

static uint32_t rng_state = 0x2545F491u;  // seed != 0

static inline uint32_t prng(void) {     // xorshift32
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}


static inline int rand_range(int min, int max) {
    return min + (int)(prng() % (uint32_t)(max - min + 1));
}

static inline uint8_t aabb_is_colliding(aabb a, aabb b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y);
}

static inline ivec2 ivec2_add(ivec2 a, ivec2 b) {
    return (ivec2){a.x + b.x, a.y + b.y};
}

#endif
