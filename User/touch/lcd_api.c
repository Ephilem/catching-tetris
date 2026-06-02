#include <stdint.h>
#include "ili_lcd_general.h"

extern const char english[][16];

void LCD_write_english(uint16_t x, uint16_t y, uint8_t str, unsigned int color, unsigned int xcolor) {
    uint8_t row, n;


    Lcd_StartRWPacket pkt = {
        .pos = {x, y},
        .size = {8, 16},
        .dir = 0
    };
    Lcd_StartReadWriteGRAM(&pkt);
    for (row = 0; row < 16; row++) {
        unsigned char bits = english[str - 32][row];

        for (n = 0; n < 8; n++) {
            Lcd_Write((bits & 0x80) ? color : xcolor);
            bits <<= 1;
        }
    }
}

void LCD_write_english_string(uint16_t x, uint16_t y, char *s, unsigned int color, unsigned int xcolor) {
    while (*s) {
        LCD_write_english(x, y, *s, color, xcolor);
        s++;
        x += 8;
    }
}
