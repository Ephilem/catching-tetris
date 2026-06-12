#include "joystick.h"

#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"


__INLINE void wait_ops(int count) {
    volatile unsigned int tmp = count;
    while (tmp--);
}

// Keypad mapping
// IO1 P0_11
// IO2 P1_20
// IO3 P1_21
// IO4 P2_13
// IO5 P2_12
static const uint8_t IO_PORT_PIN[5][2] = {
    {0, 11},
    {1, 20},
    {1, 21},
    {2, 13},
    {2, 12}
};

void Joystick_Init() {
    // make sure direction is correct
    GPIO_SetDir(0, (1 << 11), 0);
    GPIO_SetDir(1, (1 << 20) | (1 << 21), 0);
    GPIO_SetDir(2, (1 << 13) | (1 << 12), 0);

    FIO_SetMask(0, (1 << 11), 0);
    FIO_SetMask(1, (1 << 20) | (1 << 21), 0);
    FIO_SetMask(2, (1 << 13) | (1 << 12), 0);

    // pinsel necessary pins to GPIO
    PINSEL_CFG_Type pinsel;
    pinsel.Funcnum = PINSEL_FUNC_0;
    pinsel.Pinmode = PINSEL_PINMODE_PULLUP;
    pinsel.OpenDrain = PINSEL_PINMODE_NORMAL;

    pinsel.Portnum = PINSEL_PORT_2;
    pinsel.Pinnum = PINSEL_PIN_12;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_13;
    PINSEL_ConfigPin(&pinsel);

    pinsel.Portnum = PINSEL_PORT_1;
    pinsel.Pinnum = PINSEL_PIN_21;
    PINSEL_ConfigPin(&pinsel);
    pinsel.Pinnum = PINSEL_PIN_20;
    PINSEL_ConfigPin(&pinsel);

    pinsel.Portnum = PINSEL_PORT_0;
    pinsel.Pinnum = PINSEL_PIN_11;
    PINSEL_ConfigPin(&pinsel);
}

/**
 * Config the right direction and write the correct value for the pin
 * @param ioId IO1, IO2, IO3, IO4, IO5
 * @param direction 1 = set vcc, and so read the other, 0 = readable mode (default one)
 */
void Joystick_SetLineActivation(uint8_t ioId, uint8_t direction) {
    // config direction
    GPIO_SetDir(IO_PORT_PIN[ioId][0], (1 << IO_PORT_PIN[ioId][1]), direction);

    // set value
    if (direction) {
        GPIO_SetValue(IO_PORT_PIN[ioId][0], (1 << IO_PORT_PIN[ioId][1]));
    } else {
        GPIO_ClearValue(IO_PORT_PIN[ioId][0], (1 << IO_PORT_PIN[ioId][1]));
    }
}

void Joystick_ConfigureMainLine(uint8_t driveIoId) {
    uint8_t i;

    // set main line to output HIGH
    GPIO_SetDir(IO_PORT_PIN[driveIoId][0],
                (1u << IO_PORT_PIN[driveIoId][1]), 1);
    GPIO_SetValue(IO_PORT_PIN[driveIoId][0],
                  (1u << IO_PORT_PIN[driveIoId][1]));

    // set readline to input pull-down
    for (i = driveIoId + 1; i < 5; i++) {
        PINSEL_CFG_Type pinsel;
        pinsel.Funcnum = PINSEL_FUNC_0;
        pinsel.Pinmode = PINSEL_PINMODE_PULLDOWN;
        pinsel.OpenDrain = PINSEL_PINMODE_NORMAL;
        pinsel.Portnum = IO_PORT_PIN[i][0];
        pinsel.Pinnum = IO_PORT_PIN[i][1];
        PINSEL_ConfigPin(&pinsel);
        GPIO_SetDir(IO_PORT_PIN[i][0], (1u << IO_PORT_PIN[i][1]), 0);
    }
}

/**
 * Set all line to input pull-up to read the joystick input
 */
void Joystick_ConfigureGnd() {
    uint8_t i;

    for (i = 0; i < 5; i++) {
        PINSEL_CFG_Type pinsel;
        pinsel.Funcnum = PINSEL_FUNC_0;
        pinsel.Pinmode = PINSEL_PINMODE_PULLUP;
        pinsel.OpenDrain = PINSEL_PINMODE_NORMAL;
        pinsel.Portnum = IO_PORT_PIN[i][0];
        pinsel.Pinnum = IO_PORT_PIN[i][1];
        PINSEL_ConfigPin(&pinsel);
        GPIO_SetDir(IO_PORT_PIN[i][0], (1u << IO_PORT_PIN[i][1]), 0);
    }
}

/**
 * Activate one of the 4 IO, then read the value of the keys. The return value is a bitmap of the readed bits
 * If the io1, its keys 1,2,3,4 are readed in this order.
 *
 * - IO1: K1, K2, K3, K4
 * - IO2: K5, K6, K7
 * - IO3: K8, K9
 * - IO4: K10
 *
 * @param driveIoId io id
 * @return bitmap of readed bits. If the io1, its keys 1,2,3,4 are readed in this order. So if K1 and K3 are pressed, the return value will be 0b00000101
 */
uint8_t Joystick_ReadIo(uint8_t driveIoId) {
    uint8_t keys = 0;
    uint8_t i;

    Joystick_ConfigureMainLine(driveIoId);

    for (i = 0; i < 4u - driveIoId; i++) {
        wait_ops(2);
        uint8_t readIdx = driveIoId + 1 + i;
        if (GPIO_ReadValue(IO_PORT_PIN[readIdx][0]) & (1u << IO_PORT_PIN[readIdx][1])) {
            keys |= (1u << i);
        }
    }

    // reset the main line to low to avoid interference with the next read
    GPIO_ClearValue(IO_PORT_PIN[driveIoId][0], (1u << IO_PORT_PIN[driveIoId][1]));

    return keys;
}

/**
 * Read key from one to 10. This method only use Joystick_ReadIo and merge the results
 * @return little endian 1 to 10 if pressed
 */
uint16_t Joystick_ScanKeys() {
    uint16_t keys = 0;
    uint8_t ioReadKey = 0;

    // read io1 -> k1, k2, k3, k4
    ioReadKey = Joystick_ReadIo(0);
    keys |= ioReadKey;

    // read io2 -> k5, k6, k7
    ioReadKey = Joystick_ReadIo(1);
    keys |= (ioReadKey << 4);

    // read io3 -> k8, k9
    ioReadKey = Joystick_ReadIo(2);
    keys |= (ioReadKey << (4 + 3));

    // read io4 -> k10
    ioReadKey = Joystick_ReadIo(3);
    keys |= (ioReadKey << (4 + 3 + 2));

    // Reset to the default mode
    Joystick_ConfigureGnd();

    return keys;
}

static inline uint8_t Joystick_UpdateState(uint8_t cur, uint8_t active) {
    if (!active) return BTN_RELEASED;
    if (cur == BTN_RELEASED) return BTN_PRESSED;
    return BTN_HELD;
}

/**
 * Read keypad input then return the corresponding vector. Lenght between -1 and 1
 * @return
 */
void Joystick_Read(volatile Joystick_State* state) {
    uint16_t scanResult = Joystick_ScanKeys();

    uint32_t p0 = GPIO_ReadValue(0);
    uint32_t p1 = GPIO_ReadValue(1);
    uint32_t p2 = GPIO_ReadValue(2);

    // A = io2 = down
    // B = io5 = left
    // C = io4 = up
    // D = io1 = right
    // PRESS = io3
    uint8_t up = (p2 >> 13) & 1;
    uint8_t down = (p1 >> 20) & 1;
    uint8_t right = (p2 >> 12) & 1;
    uint8_t left = (p0 >> 11) & 1;


    state->dir.x = (int16_t) (right - left);
    state->dir.y = (int16_t) (up - down);

    state->pressed = Joystick_UpdateState(state->pressed, !((p1 >> 21) & 1));

    state->k1 = Joystick_UpdateState(state->k1, (scanResult >> 0) & 1);
    state->k2 = Joystick_UpdateState(state->k2, (scanResult >> 1) & 1);
    state->k3 = Joystick_UpdateState(state->k2, (scanResult >> 2) & 1);
    state->k4 = Joystick_UpdateState(state->k2, (scanResult >> 3) & 1);
    state->k5 = Joystick_UpdateState(state->k2, (scanResult >> 4) & 1);
    state->k6 = Joystick_UpdateState(state->k2, (scanResult >> 5) & 1);
    state->k7 = Joystick_UpdateState(state->k2, (scanResult >> 6) & 1);
    state->k8 = Joystick_UpdateState(state->k2, (scanResult >> 7) & 1);
    state->k9 = Joystick_UpdateState(state->k2, (scanResult >> 8) & 1);
    state->k10 = Joystick_UpdateState(state->k2, (scanResult >> 9) & 1);

}
