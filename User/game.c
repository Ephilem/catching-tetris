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

void Game_MoveMass(ivec2 delta) {
    ivec2 nextPos = {
        gameState.massGrid.pos.x + delta.x,
        gameState.massGrid.pos.y + delta.y
    };
    Game_SetMassPosition(nextPos);
}

void Game_UpdateUserInput(ivec2 currDir) {
    Game_UserInput* input = &gameState.userInput;

    ivec2 dir = {0,0};
    if (currDir.x != 0 && currDir.y != 0) {
        // remove diagonal input
        dir = (ivec2){currDir.x, 0};
    } else {
        dir = currDir;
    }

    // if dir changed, full reset
    if (dir.x != input->lockedDir.x || dir.y != input->lockedDir.y) {
        input->lockedDir = dir;
        input->dasState = INPUT_DAS_STATE_DAS;
        input->dasCpt = 0;
        input->arrCpt = 0;

        // if the new direction is not idle, move now
        if (input->lockedDir.x != 0 || input->lockedDir.y != 0) {
            Game_MoveMass(input->lockedDir);
        } else {
            input->dasState = INPUT_DAS_STATE_IDLE;
        }
    }

    // ticks system
    if (input->dasState == INPUT_DAS_STATE_IDLE) return;

    if (input->dasState == INPUT_DAS_STATE_DAS) {
        input->dasCpt++;
        if (input->dasCpt >= DAS_THRESHOLD) {
            input->dasState = INPUT_DAS_STATE_ARR;
            input->dasCpt = 0;
            Game_MoveMass(input->lockedDir);
        }
    }

    if (input->dasState == INPUT_DAS_STATE_ARR) {
        input->arrCpt++;
        if (input->arrCpt >= ARR_THRESHOLD) {
            input->arrCpt = 0;
            Game_MoveMass(input->lockedDir);
        }
    }
}

void Game_SpawnRandomPiece() {

}

void Game_ApplyGravity() {
    int i;

    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        Game_FallingPiece* piece = &gameState.fallingPieces[i];
        if (!piece->active) continue;

        // try to move down
        piece->pos.y += 1;

        // TODO collision test with mass, if collision, move back and set piece as inactive and add its blocks to the mass
    }
}

void Game_Init() {
    Game_SetMassBlock(0, 0, MASS_CORE);
    Game_SetMassBlock(1, 0, MASS_SOLID);
    Game_SetMassBlock(-1, 0, MASS_SOLID);
    Game_SetMassBlock(0, 1, MASS_SOLID);
    Game_SetMassBlock(0, -1, MASS_SOLID);
    Game_UpdateMassAabb();

    Game_SetMassPosition((ivec2){GLOBAL_GRID_W/2, GLOBAL_GRID_H/2});

    gameState.fallingPieces[0] = (Game_FallingPiece){
        .active = 1,
        .pos = {3, 3},
        .type = 2,
        .rotation = 3
    };

    // initial render
    Render_FlagMassAsDirty();
}
