#include "game.h"

#include "render.h"

Game_State gameState = {0};

void Game_SetMassBlock(int8_t x, int8_t y, uint8_t mass) {
    uint8_t (*massGrid)[MASS_GRID_W][MASS_GRID_H] = &gameState.massGrid.grid;
    (*massGrid)[x+MASS_GRID_W_HALF][y+MASS_GRID_H_HALF] = mass;
}

__INLINE uint8_t Game_ReadMassBlock(int8_t x, int8_t y) {
    uint8_t (*massGrid)[MASS_GRID_W][MASS_GRID_H] = &gameState.massGrid.grid;
    return (*massGrid)[x+MASS_GRID_W_HALF][y+MASS_GRID_H_HALF];
}

void Game_UpdateMassAabb() {
    // calculate aabb
    int8_t mx, my;
    aabb box = {127, 127, -127, -127};

    // TODO meilleur façon de faire surement
    for (mx = -MASS_GRID_W_HALF; mx <= MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my <= MASS_GRID_H_HALF; my++) {
            if (gameState.massGrid.grid[mx+MASS_GRID_W_HALF][my+MASS_GRID_H_HALF] == MASS_EMPTY) continue;
            if (mx < box.min.x) box.min.x = mx;
            if (mx > box.max.x) box.max.x = mx;
            if (my < box.min.y) box.min.y = my;
            if (my > box.max.y) box.max.y = my;
        }
    }

    gameState.massGrid.aabb = box;
}

__INLINE aabb Game_GetMassAbsoluteAabb() {
    return (aabb){
        .min = {
            gameState.massGrid.aabb.min.x + gameState.massGrid.pos.x,
            gameState.massGrid.aabb.min.y + gameState.massGrid.pos.y
        },
        .max = {
            gameState.massGrid.aabb.max.x + gameState.massGrid.pos.x,
            gameState.massGrid.aabb.max.y + gameState.massGrid.pos.y
        }
    };
}

void Game_SetMassPosition(ivec2 pos) {
    // clamp pose with aabb
    ivec2 currPos = gameState.massGrid.pos;
    aabb box = gameState.massGrid.aabb;
    if (pos.x + box.min.x < 0 || pos.x + box.max.x >= GLOBAL_GRID_W)
        pos.x = currPos.x;
    if (pos.y + box.min.y < 0 || pos.y + box.max.y >= GLOBAL_GRID_H)
        pos.y = currPos.y;

    gameState.massGrid.pos = pos;

    Render_FlagMassAsDirty();
}

void Game_Init() {
    Game_SetMassBlock(0, 0, MASS_CORE);
    Game_SetMassBlock(1, 0, MASS_SOLID);
    Game_SetMassBlock(-1, 0, MASS_SOLID);
    Game_SetMassBlock(0, 1, MASS_SOLID);
    Game_SetMassBlock(0, -1, MASS_SOLID);

    Game_SetMassBlock(0, -2, MASS_SOLID);
    Game_SetMassBlock(0, -3, MASS_SOLID);
    Game_SetMassBlock(0, -4, MASS_SOLID);
    Game_SetMassBlock(0, -5, MASS_SOLID);
    Game_UpdateMassAabb();

    Game_SetMassPosition((ivec2){GLOBAL_GRID_W/2, GLOBAL_GRID_H/2});

    // initial render
    Render_FlagMassAsDirty();
}
