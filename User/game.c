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

void Game_CalculatePieceAabb(Game_FallingPiece* piece) {
    aabb box = {3, 3, 0, 0};
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;
            if (tx < box.min.x) box.min.x = tx;
            if (tx > box.max.x) box.max.x = tx;
            if (ty < box.min.y) box.min.y = ty;
            if (ty > box.max.y) box.max.y = ty;
        }
    }

    piece->aabb = box;
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

__INLINE aabb Game_CalculateAbsoluteAabb(const ivec2 *pos, const aabb *localAabb) {
    return (aabb){
        .min = {
            localAabb->min.x + pos->x,
            localAabb->min.y + pos->y
        },
        .max = {
            localAabb->max.x + pos->x,
            localAabb->max.y + pos->y
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
    // find a avalaible place
    int i;
    Game_FallingPiece* p = NULL;
    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        if (!gameState.fallingPieces[i].active) {
            p = &gameState.fallingPieces[i];
            break;
        }
    }

    if (!p) return;

    p->active = 1;
    p->type = rand_range(0, 6);
    p->rotation = rand_range(0, 3);
    p->pos = (ivec2){GLOBAL_GRID_W - 3, -3};
    Game_CalculatePieceAabb(p);
}

uint8_t Game_TestCollisionWithMass(const Game_FallingPiece* piece, ivec2 nextHop) {
    // first test: aabb test
    aabb pieceBox = Game_CalculateAbsoluteAabb(&piece->pos, &piece->aabb);
    aabb massBox  = Game_CalculateAbsoluteAabb(&gameState.massGrid.pos, &gameState.massGrid.aabb);
    if (!aabb_is_colliding(pieceBox, massBox)) return 0;

    // second test: presice collision test
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;

            // test collision with mass
            int8_t cellX = piece->pos.x + tx + nextHop.x;
            int8_t cellY = piece->pos.y + ty + nextHop.y;
            if (Game_ReadMassBlock(cellX - gameState.massGrid.pos.x, cellY - gameState.massGrid.pos.y) != MASS_EMPTY) {
                return 1;
            }
        }
    }

    return 0;
}

void Game_FusePiece(Game_FallingPiece* piece) {
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;

            int8_t cellX = piece->pos.x + tx;
            int8_t cellY = piece->pos.y + ty;
            Game_SetMassBlock(cellX - gameState.massGrid.pos.x, cellY - gameState.massGrid.pos.y, MASS_SOLID);
        }
    }

    Game_UpdateMassAabb();
}

void Game_ApplyGravity() {
    int i;

    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        Game_FallingPiece* piece = &gameState.fallingPieces[i];
        if (!piece->active) continue;

        ivec2 nextHop = {0, 1};
        if (Game_TestCollisionWithMass(piece, nextHop)) {
            Game_FusePiece(piece);
        } else {
            piece->pos.y += 1;
        }
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
    Game_CalculatePieceAabb(&gameState.fallingPieces[0]);

    // initial render
    Render_FlagMassAsDirty();
}
