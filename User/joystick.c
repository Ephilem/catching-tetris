#include "joystick.h"

#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

// Config GPIO
// P2.8  = KEY_A
// P2.12 = KEY_B
// P2.13 = KEY_C
// P1.21 = KEY_D
// P1.20 = KEY_CTL
// P2.11 = KEY_1
// P2.10 = KEY_2
void Joystick_Init() {
    // make sure direction is correct
    GPIO_SetDir(2, (1 << 8) | (1 << 12) | (1 << 13), 0);
    GPIO_SetDir(1, (1 << 20) | (1 << 21) | (1 << 11) | (1 << 10), 0);

    FIO_SetMask(2, (1 << 8) | (1 << 12) | (1 << 13), 0);
    FIO_SetMask(1, (1 << 20) | (1 << 21) | (1 << 11) | (1 << 10), 0);

    // pinsel necessary pins to GPIO
    PINSEL_CFG_Type pinsel;
    pinsel.Funcnum = PINSEL_FUNC_0;
    pinsel.Pinmode = PINSEL_PINMODE_PULLUP;
    pinsel.OpenDrain = PINSEL_PINMODE_NORMAL;

    pinsel.Portnum = PINSEL_PORT_2;
    pinsel.Pinnum = PINSEL_PIN_8;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_10;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_11;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_12;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_13;
    PINSEL_ConfigPin(&pinsel);

    pinsel.Portnum = PINSEL_PORT_1;
    pinsel.Pinnum = PINSEL_PIN_21;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_20;
    PINSEL_ConfigPin(&pinsel);
}

/**
 * Read joystick input then return the corresponding vector. Lenght between -1 and 1
 * @return
 */
void Joystick_Read(volatile Joystick_State *state) {
    uint32_t port1 = GPIO_ReadValue(1);
    uint32_t port2 = GPIO_ReadValue(2);

    //           KEY_A         -       KEY_D
    int x = ((port2 >> 8) & 1) - ((port1 >> 21) & 1);
    //           KEY_B         -       KEY_C
    int y = ((port2 >> 12) & 1) - ((port2 >> 13) & 1);

    state->dir = (ivec2) {x, y};

    uint8_t pressed = !((port1 >> 20) & 1);
    if (!pressed) {
        state->pressed = BTN_RELEASED;
    } else if (state->pressed == BTN_RELEASED) {
        state->pressed = BTN_PRESSED;
    } else {
        state->pressed = BTN_HELD;
    }

    uint8_t k1 = !((port2 >> 11) & 1);
    uint8_t k2 = !((port2 >> 10) & 1);

    if (!k1) {
        state->key1 = BTN_RELEASED;
    } else if (state->key1 == BTN_RELEASED) {
        state->key1 = BTN_PRESSED;
    } else {
        state->key1 = BTN_HELD;
    }

    if (!k2) {
        state->key2 = BTN_RELEASED;
    } else if (state->key2 == BTN_RELEASED) {
        state->key2 = BTN_PRESSED;
    } else {
        state->key2 = BTN_HELD;
    }
}
