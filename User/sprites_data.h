#ifndef SPRITES_DATA_H
#define SPRITES_DATA_H
#include <stdint.h>

typedef struct {
    const unsigned short* data;
    uint16_t width;
    uint16_t height;
} Sprite;

extern const Sprite SPT_CyanCube;
extern const Sprite SPT_YellowCube;
extern const Sprite SPT_MagentaCube;
extern const Sprite SPT_GreenCube;
extern const Sprite SPT_RedCube;
extern const Sprite SPT_BlueCube;
extern const Sprite SPT_OrangeCube;

extern const Sprite SPT_GoldCube;

#endif