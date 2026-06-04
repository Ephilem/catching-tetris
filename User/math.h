#ifndef MATH_H
#define MATH_H

#include <stdint.h>

typedef struct {
    int16_t x;
    int16_t y;
} ivec2;

typedef struct {
    ivec2 min;
    ivec2 max;
} aabb;

static uint32_t rng_state = 0x2545F491u; // seed != 0

static inline uint32_t prng(void) {
    // xorshift32
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}


static inline int rand_range(int min, int max) {
    return min + (int) (prng() % (uint32_t) (max - min + 1));
}

static inline uint8_t aabb_is_colliding(aabb a, aabb b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y);
}

static inline ivec2 ivec2_add(ivec2 a, ivec2 b) {
    return (ivec2){a.x + b.x, a.y + b.y};
}

#define ROT_FULL_STEPS    64
#define ROT_QUARTER_STEPS 16

static const int16_t SIN_Q8_QUARTER[ROT_QUARTER_STEPS + 1] = {
    0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255, 256
};

static inline int16_t sin_q8(int step) {
    step &= (ROT_FULL_STEPS - 1);
    if (step <= ROT_QUARTER_STEPS) return SIN_Q8_QUARTER[step];
    if (step <= 2 * ROT_QUARTER_STEPS) return SIN_Q8_QUARTER[2 * ROT_QUARTER_STEPS - step];
    if (step <= 3 * ROT_QUARTER_STEPS) return -SIN_Q8_QUARTER[step - 2 * ROT_QUARTER_STEPS];
    return -SIN_Q8_QUARTER[ROT_FULL_STEPS - step];
}

static inline int16_t cos_q8(int step) {
    return sin_q8(step + ROT_QUARTER_STEPS);
}


static inline int floordiv8(int v) {
    return (v >= 0) ? (v / 8) : -((7 - v) / 8);
}

// from https://gist.github.com/foobaz/3287f153d125277eefea
static uint16_t isqrt32(uint32_t x) {
    uint32_t rem = 0, root = 0;
    int i;

    for (i = 32 / 2; i > 0; i--) {
        root <<= 1;
        rem = (rem << 2) | (x >> (32 - 2));
        x <<= 2;
        if (root < rem) {
            rem -= root | 1;
            root += 2;
        }
    }

    return root >> 1;
}


#endif
