#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

#include "math.h"

#define BTN_RELEASED 0
#define BTN_PRESSED 1
#define BTN_HELD 2

typedef struct {
    ivec2 dir;
    uint8_t pressed : 2;

    uint8_t k1 : 2;
    uint8_t k2 : 2;
    uint8_t k3  : 2;
    uint8_t k4  : 2;

    uint8_t k5  : 2;
    uint8_t k6  : 2;
    uint8_t k7  : 2;
    uint8_t k8  : 2;

    uint8_t k9  : 2;
    uint8_t k10 : 2;
} Joystick_State;

void Joystick_Init();
void Joystick_Read(volatile Joystick_State *state);

#endif


