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

/**
 * flags changed by the timer or other interrupt to communicate things to the main gameloop
 */
typedef struct {
    volatile uint8_t cpt_update;

    volatile uint8_t flag_update : 1;
} Game_IRQFlags;

typedef struct {
    uint8_t grid[MASS_GRID_W][MASS_GRID_H];
    ivec2 pos;
    aabb aabb;
} Game_MassGrid;

typedef struct {
    Game_IRQFlags irqFlags;
    Joystick_State joystickState;
    Game_MassGrid massGrid;
} Game_State;

extern Game_State gameState;

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
void Game_SetMassPosition(ivec2 pos);

void Game_Init();

#endif
