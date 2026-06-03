#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#include "joystick.h"
#include "ili_lcd_general.h"

#define GLOBAL_GRID_W (LCD_HEIGHT / 8)
#define GLOBAL_GRID_H (LCD_WIDTH / 8)
#define MASS_GRID_W 21
#define MASS_GRID_H 21
#define MASS_GRID_W_HALF (MASS_GRID_W / 2)
#define MASS_GRID_H_HALF (MASS_GRID_H / 2)

#define MASS_EMPTY 0
#define MASS_CORE 1
#define MASS_SOLID 2

// val * 10ms best with ARR > render time
#define DAS_THRESHOLD 19
#define ARR_THRESHOLD 5

#define MAX_FALLING_PIECES 4
#define GRAVITY_SPEED 15 // each 100ms

#define PIECE_I 0
#define PIECE_O 1
#define PIECE_T 2
#define PIECE_S 3
#define PIECE_Z 4
#define PIECE_J 5
#define PIECE_L 6

/**
 * flags changed by the timer or other interrupt to communicate things to the main gameloop
 */
typedef struct {
    volatile uint8_t cpt_render;
    volatile uint8_t cpt_tick;

    volatile uint8_t flag_render: 1;
    volatile uint8_t flag_tick: 1;
} Game_IRQFlags;


typedef struct {
    uint8_t grid[MASS_GRID_W][MASS_GRID_H];
    ivec2 pos;
    aabb aabb;
} Game_MassGrid;

typedef struct {
    uint8_t active; // differenciate array cell with real data in it

    uint8_t type; // 0-6
    uint8_t rotation;

    ivec2 pos; // up left of the 4x4 piece grid
} Game_FallingPiece;

/**
 * Data for user control of the game (DAS, ARR). Not the same as Joystick_State, which is an translation of the raw joystick input
 */
#define INPUT_DAS_STATE_IDLE 0
#define INPUT_DAS_STATE_DAS 1
#define INPUT_DAS_STATE_ARR 2

typedef struct {
    ivec2 lockedDir;
    uint8_t dasState;
    uint8_t dasCpt;
    uint8_t arrCpt;
} Game_UserInput;

typedef struct {
    Joystick_State joystickState;

    Game_IRQFlags irqFlags;

    Game_UserInput userInput;
    Game_MassGrid massGrid;

    uint8_t gravityCpt;
    Game_FallingPiece fallingPieces[MAX_FALLING_PIECES];
} Game_State;

extern Game_State gameState;
static const uint8_t TETROMINOS[7][4][4][4];

/**
 * Set the mass of a block relative to the core at the center of the grid. Will also update the position of the mass if the block is the core
 * @param x relative coord x
 * @param y relative coord y
 * @param mass id
 */
void Game_SetMassBlock(int8_t x, int8_t y, uint8_t mass);

__INLINE uint8_t Game_ReadMassBlock(int8_t x, int8_t y);

/**
 * Update AABB that fit the mass for border collision checking
 */
void Game_UpdateMassAabb();
void Game_MoveMass(ivec2 delta);
void Game_SetMassPosition(ivec2 pos);

void Game_SpawnRandomPiece();

/**
 * bring down all pieces, and test for collision with the mass
 */
void Game_ApplyGravity();

/**
 * update the user input state
 */
void Game_UpdateUserInput(ivec2 currDir);

void Game_Init();


///////////////

// [type][rotation][y][x]
static const uint8_t TETROMINOS[7][4][4][4] = {

    // I
    {
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0}
        },
        {
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        },
    },

    // O
    {
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
        {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}},
        {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}},
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    },

    // T
    {
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0}
        },
    },

    // S
    {
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {1, 1, 0, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {1, 1, 0, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 0}
        },
    },

    // Z
    {
        {
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 1, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 1, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0}
        },
    },

    // J
    {
        {
            {0, 0, 0, 0},
            {1, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {0, 0, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {1, 1, 0, 0}
        },
    },

    // L
    {
        {
            {0, 0, 0, 0},
            {0, 0, 1, 0},
            {1, 1, 1, 0},
            {0, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0}
        },
        {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 1, 1, 0},
            {1, 0, 0, 0}
        },
        {
            {0, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 0, 0}
        },
    },
};

#endif
