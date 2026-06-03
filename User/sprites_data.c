#include "sprites_data.h"
#include "ili_lcd_general.h"

#define GRE1 RGB565(0, 128, 0)
#define GRE2 RGB565(0, 160, 0)
#define GRE3 RGB565(0, 208, 0)
#define GRE4 RGB565(64, 240, 56)
#define GRE5 RGB565(128, 248, 128)

#define YEL1 RGB565(144, 128, 0)
#define YEL2 RGB565(200, 192, 16)
#define YEL3 RGB565(248, 208, 0)
#define YEL4 RGB565(248, 216, 80)
#define YEL5 RGB565(248, 248, 160)

#define BLU1 RGB565(16, 16, 128)
#define BLU2 RGB565(40, 48, 216)
#define BLU3 RGB565(96, 96, 246)
#define BLU4 RGB565(144, 144, 248)
#define BLU5 RGB565(192, 192, 248)

#define CYA1 RGB565(0, 96, 136)
#define CYA2 RGB565(0, 152, 192)
#define CYA3 RGB565(32, 200, 248)
#define CYA4 RGB565(96, 232, 248)
#define CYA5 RGB565(160, 248, 248)

#define ORA1 RGB565(144, 56, 0)
#define ORA2 RGB565(200, 88, 8)
#define ORA3 RGB565(248, 120, 32)
#define ORA4 RGB565(248, 160, 56)
#define ORA5 RGB565(248, 216, 96)

#define MAG1 RGB565(144, 0, 144)
#define MAG2 RGB565(163, 0, 170)
#define MAG3 RGB565(216, 40, 216)
#define MAG4 RGB565(232, 96, 232)
#define MAG5 RGB565(248, 160, 248)

#define RED1 RGB565(160, 0, 0)
#define RED2 RGB565(200, 8, 8)
#define RED3 RGB565(240, 24, 24)
#define RED4 RGB565(240, 64, 64)
#define RED5 RGB565(248, 104, 104)


#define GOL1 RGB565(121, 40, 0)
#define GOL2 RGB565(195, 89, 32)
#define GOL3 RGB565(219, 162, 16)
#define GOL4 RGB565(251, 195, 40)
#define GOL5 RGB565(251, 219, 73)
#define GOL6 RGB565(251, 251, 48)

#define WHI1 White

// -------- CUBES ----------
#define SPT_Cube_W 8
#define SPT_Cube_H 8

const unsigned short SPTData_GreenCube[SPT_Cube_W * SPT_Cube_H] = {
    GRE1, GRE3, GRE3, GRE3, GRE3, GRE3, GRE3, GRE4,
    GRE1, GRE5, GRE5, GRE5, GRE5, GRE5, GRE5, GRE4,
    GRE1, GRE5, GRE5, GRE5, GRE5, GRE5, GRE4, GRE4,
    GRE1, GRE5, GRE5, GRE5, GRE4, GRE4, GRE4, GRE4,
    GRE1, GRE5, GRE4, GRE3, GRE3, GRE4, GRE4, GRE4,
    GRE1, GRE5, GRE4, GRE3, GRE3, GRE4, GRE5, GRE4,
    GRE1, GRE5, GRE4, GRE4, GRE4, GRE5, GRE5, GRE4,
    GRE1, GRE2, GRE2, GRE2, GRE2, GRE2, GRE2, GRE3,
};
const Sprite SPT_GreenCube = {
    .data = SPTData_GreenCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};

const unsigned short SPTData_YellowCube[SPT_Cube_W * SPT_Cube_H] = {
    YEL1, YEL3, YEL3, YEL3, YEL3, YEL3, YEL3, YEL4,
    YEL1, YEL5, YEL5, YEL5, YEL5, YEL5, YEL5, YEL4,
    YEL1, YEL5, YEL5, YEL5, YEL5, YEL5, YEL4, YEL4,
    YEL1, YEL5, YEL5, YEL5, YEL4, YEL4, YEL4, YEL4,
    YEL1, YEL5, YEL4, YEL3, YEL3, YEL4, YEL4, YEL4,
    YEL1, YEL5, YEL4, YEL3, YEL3, YEL4, YEL5, YEL4,
    YEL1, YEL5, YEL4, YEL4, YEL4, YEL5, YEL5, YEL4,
    YEL1, YEL2, YEL2, YEL2, YEL2, YEL2, YEL2, YEL3,
};
const Sprite SPT_YellowCube = {
    .data = SPTData_YellowCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};

const unsigned short SPTData_BlueCube[SPT_Cube_W * SPT_Cube_H] = {
    BLU1, BLU3, BLU3, BLU3, BLU3, BLU3, BLU3, BLU4,
    BLU1, BLU5, BLU5, BLU5, BLU5, BLU5, BLU5, BLU4,
    BLU1, BLU5, BLU5, BLU5, BLU5, BLU5, BLU4, BLU4,
    BLU1, BLU5, BLU5, BLU5, BLU4, BLU4, BLU4, BLU4,
    BLU1, BLU5, BLU4, BLU3, BLU3, BLU4, BLU4, BLU4,
    BLU1, BLU5, BLU4, BLU3, BLU3, BLU4, BLU5, BLU4,
    BLU1, BLU5, BLU4, BLU4, BLU4, BLU5, BLU5, BLU4,
    BLU1, BLU2, BLU2, BLU2, BLU2, BLU2, BLU2, BLU3,
};
const Sprite SPT_BlueCube = {
    .data = SPTData_BlueCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};

const unsigned short SPTData_CyanCube[SPT_Cube_W * SPT_Cube_H] = {
    CYA1, CYA3, CYA3, CYA3, CYA3, CYA3, CYA3, CYA4,
    CYA1, CYA5, CYA5, CYA5, CYA5, CYA5, CYA5, CYA4,
    CYA1, CYA5, CYA5, CYA5, CYA5, CYA5, CYA4, CYA4,
    CYA1, CYA5, CYA5, CYA5, CYA4, CYA4, CYA4, CYA4,
    CYA1, CYA5, CYA4, CYA3, CYA3, CYA4, CYA4, CYA4,
    CYA1, CYA5, CYA4, CYA3, CYA3, CYA4, CYA5, CYA4,
    CYA1, CYA5, CYA4, CYA4, CYA4, CYA5, CYA5, CYA4,
    CYA1, CYA2, CYA2, CYA2, CYA2, CYA2, CYA2, CYA3,
};
const Sprite SPT_CyanCube = {
    .data = SPTData_CyanCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};

const unsigned short SPTData_RedCube[SPT_Cube_W * SPT_Cube_H] = {
    RED1, RED3, RED3, RED3, RED3, RED3, RED3, RED4,
    RED1, RED5, RED5, RED5, RED5, RED5, RED5, RED4,
    RED1, RED5, RED5, RED5, RED5, RED5, RED4, RED4,
    RED1, RED5, RED5, RED5, RED4, RED4, RED4, RED4,
    RED1, RED5, RED4, RED3, RED3, RED4, RED4, RED4,
    RED1, RED5, RED4, RED3, RED3, RED4, RED5, RED4,
    RED1, RED5, RED4, RED4, RED4, RED5, RED5, RED4,
    RED1, RED2, RED2, RED2, RED2, RED2, RED2, RED3,
};
const Sprite SPT_RedCube = {
    .data = SPTData_RedCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};

const unsigned short SPTData_OrangeCube[SPT_Cube_W * SPT_Cube_H] = {
    ORA1, ORA3, ORA3, ORA3, ORA3, ORA3, ORA3, ORA4,
    ORA1, ORA5, ORA5, ORA5, ORA5, ORA5, ORA5, ORA4,
    ORA1, ORA5, ORA5, ORA5, ORA5, ORA5, ORA4, ORA4,
    ORA1, ORA5, ORA5, ORA5, ORA4, ORA4, ORA4, ORA4,
    ORA1, ORA5, ORA4, ORA3, ORA3, ORA4, ORA4, ORA4,
    ORA1, ORA5, ORA4, ORA3, ORA3, ORA4, ORA5, ORA4,
    ORA1, ORA5, ORA4, ORA4, ORA4, ORA5, ORA5, ORA4,
    ORA1, ORA2, ORA2, ORA2, ORA2, ORA2, ORA2, ORA3,
};
const Sprite SPT_OrangeCube = {
    .data = SPTData_OrangeCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};

const unsigned short SPTData_MagentaCube[SPT_Cube_W * SPT_Cube_H] = {
    MAG1, MAG3, MAG3, MAG3, MAG3, MAG3, MAG3, MAG4,
    MAG1, MAG5, MAG5, MAG5, MAG5, MAG5, MAG5, MAG4,
    MAG1, MAG5, MAG5, MAG5, MAG5, MAG5, MAG4, MAG4,
    MAG1, MAG5, MAG5, MAG5, MAG4, MAG4, MAG4, MAG4,
    MAG1, MAG5, MAG4, MAG3, MAG3, MAG4, MAG4, MAG4,
    MAG1, MAG5, MAG4, MAG3, MAG3, MAG4, MAG5, MAG4,
    MAG1, MAG5, MAG4, MAG4, MAG4, MAG5, MAG5, MAG4,
    MAG1, MAG2, MAG2, MAG2, MAG2, MAG2, MAG2, MAG3,
};
const Sprite SPT_MagentaCube = {
    .data = SPTData_MagentaCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};



const unsigned short SPTData_GoldCube[SPT_Cube_W * SPT_Cube_H] = {
    GOL4, GOL5, GOL5, GOL5, GOL5, GOL5, GOL5, GOL2,
    GOL3, WHI1, GOL6, GOL6, GOL6, GOL6, GOL5, GOL1,
    GOL3, GOL4, WHI1, GOL6, GOL6, GOL5, GOL2, GOL1,
    GOL3, GOL4, GOL4, WHI1, GOL5, GOL2, GOL2, GOL1,
    GOL3, GOL4, GOL4, GOL3, GOL2, GOL2, GOL2, GOL1,
    GOL3, GOL4, GOL3, GOL1, GOL1, GOL2, GOL2, GOL1,
    GOL3, GOL3, GOL1, GOL1, GOL1, GOL1, GOL2, GOL1,
    GOL2, GOL1, GOL1, GOL1, GOL1, GOL1, GOL1, GOL1,
};
const Sprite SPT_GoldCube = {
    .data = SPTData_GoldCube,
    .width = SPT_Cube_W,
    .height = SPT_Cube_H
};