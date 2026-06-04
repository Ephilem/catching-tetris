#include "game.h"

#include <string.h>

#include "render.h"

Game_State gameState = {0};


void Game_SetMassBlock(int8_t x, int8_t y, uint8_t mass) {
    int ix = x + MASS_GRID_W_HALF;
    int iy = y + MASS_GRID_H_HALF;
    if (ix < 0 || ix >= MASS_GRID_W || iy < 0 || iy >= MASS_GRID_H) return;
    gameState.massGrid.grid[ix][iy] = mass;

}

__INLINE uint8_t Game_ReadMassBlock(int8_t x, int8_t y) {
    int ix = x + MASS_GRID_W_HALF;
    int iy = y + MASS_GRID_H_HALF;
    if (ix < 0 || ix >= MASS_GRID_W || iy < 0 || iy >= MASS_GRID_H) return MASS_EMPTY;
    return gameState.massGrid.grid[ix][iy];
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
    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
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

    ivec2 delta = {
        pos.x - currPos.x,
        pos.y - currPos.y
    };
    gameState.massGrid.pos = pos;


    // test for collision
    int i = 0;
    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        Game_FallingPiece* piece = &gameState.fallingPieces[i];
        if (!piece->active) continue;

        if (Game_TestCollisionWithMass(piece, (ivec2){0,0})) {
            Render_ErasePiece(piece);

            // before fuse, we need to move the piece in the direction of the movement
            piece->pos = (ivec2){
                piece->pos.x + delta.x,
                piece->pos.y + delta.y
            };
            Game_FusePiece(piece);
            break;
        }
    }

    Render_FlagMassAsDirty();
}

void Game_MoveMass(ivec2 delta) {
    ivec2 nextPos = {
        gameState.massGrid.pos.x + delta.x,
        gameState.massGrid.pos.y + delta.y
    };
    Game_SetMassPosition(nextPos);
}

/**
 * Explore the grid and mark as critial when 4x4 is found
 */
void Game_SearchFor4x4() {
    int mx, my;
    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
            if (Game_ReadMassBlock(mx, my) == MASS_EMPTY) continue;

            // check 4x4
            int dx, dy;
            uint8_t found4x4 = 1;
            for (dx = 0; dx < 4 && found4x4; dx++) {
                for (dy = 0; dy < 4 && found4x4; dy++) {
                    if (Game_ReadMassBlock(mx + dx, my + dy) == MASS_EMPTY) {
                        found4x4 = 0;
                    }
                }
            }

            if (found4x4) {
                for (dx = 0; dx < 4; dx++) {
                    for (dy = 0; dy < 4; dy++) {
                        uint8_t v = Game_ReadMassBlock(mx + dx, my + dy);
                        if (v == MASS_CORE || v == MASS_CORE_CRITICAL)
                            Game_SetMassBlock(mx + dx, my + dy, MASS_CORE_CRITICAL);
                        else
                            Game_SetMassBlock(mx + dx, my + dy, MASS_CRITICAL);

                    }
                }
            }
        }
    }
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

void Game_UpdateRotationInput(const Joystick_State* js) {
    if (gameState.massGrid.rotating) return;
    if (js->key1 == BTN_PRESSED) {
        gameState.massGrid.rotTarget -= ROT_QUARTER_STEPS;
        gameState.massGrid.rotating = 1;
    }
    if (js->key2 == BTN_PRESSED) {
        gameState.massGrid.rotTarget += ROT_QUARTER_STEPS;
        gameState.massGrid.rotating = 1;
    }
}

void Game_RotateMassQuarter(int quarters) {
    static uint8_t rotTmpGrid[MASS_GRID_W][MASS_GRID_H]; // buffer static so its isn't in the pile
    const int dir = quarters > 0 ? 1 : -1;
    int n = quarters > 0 ? quarters : -quarters;
    int8_t mx, my;

    while (n-- > 0) {
        memset(rotTmpGrid, MASS_EMPTY, sizeof(rotTmpGrid));

        for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
            for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
                uint8_t v = gameState.massGrid.grid[mx + MASS_GRID_W_HALF][my + MASS_GRID_H_HALF];
                if (v == MASS_EMPTY) continue;

                int nx, ny;
                if (dir > 0) { nx = -my; ny =  mx; }
                else         { nx =  my; ny = -mx; }

                int ix = nx + MASS_GRID_W_HALF;
                int iy = ny + MASS_GRID_H_HALF;

                if (ix < 0 || ix >= MASS_GRID_W || iy < 0 || iy >= MASS_GRID_H) continue;
                rotTmpGrid[ix][iy] = v;
            }
        }

        memcpy(gameState.massGrid.grid, rotTmpGrid, sizeof(rotTmpGrid));
    }

    Game_UpdateMassAabb();

    // wall kick. move the mass if outside the grid after rotation
    aabb b = gameState.massGrid.aabb;
    ivec2 pos = gameState.massGrid.pos;
    if (pos.x + b.min.x < 0)              pos.x = -b.min.x;
    if (pos.x + b.max.x >= GLOBAL_GRID_W) pos.x = GLOBAL_GRID_W - 1 - b.max.x;
    if (pos.y + b.min.y < 0)              pos.y = -b.min.y;
    if (pos.y + b.max.y >= GLOBAL_GRID_H) pos.y = GLOBAL_GRID_H - 1 - b.max.y;
    Game_SetMassPosition(pos);
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
    // p->type = 0;
    // p->rotation = 3;
    p->pos = (ivec2){rand_range(2, GLOBAL_GRID_W-5), -3};
    Game_CalculatePieceAabb(p);
}

uint8_t Game_TestCollisionWithMass(const Game_FallingPiece* piece, ivec2 nextHop) {
    // first test: aabb test
    ivec2 nextPos = ivec2_add(piece->pos, nextHop);
    aabb pieceBox = Game_CalculateAbsoluteAabb(&nextPos, &piece->aabb);
    aabb massBox  = Game_CalculateAbsoluteAabb(&gameState.massGrid.pos, &gameState.massGrid.aabb);
    if (!aabb_is_colliding(pieceBox, massBox)) return 0;

    // second test: presice collision test
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;

            // test collision with mass
            int16_t cellX = nextPos.x + tx;
            int16_t cellY = nextPos.y + ty;
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

    piece->active = 0;
    Game_UpdateMassAabb();
    Render_FlagMassAsDirty();
    Game_SearchFor4x4();
}

static uint8_t Game_TestCollisionWithRotatingMass(const Game_FallingPiece* piece) {
    const int16_t cos_v = cos_q8(gameState.massGrid.rotCur);
    const int16_t sin_v = sin_q8(gameState.massGrid.rotCur);
    const ivec2 pos = gameState.massGrid.pos;
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];

    int8_t tx, ty;
    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;

            int16_t px = (piece->pos.x + tx - pos.x) * 8;
            int16_t py = (piece->pos.y + ty - pos.y) * 8;

            // same logic for rendering
            int16_t sx = ( cos_v * px + sin_v * py) / 256;
            int16_t sy = (-sin_v * px + cos_v * py) / 256;

            int16_t mx = floordiv8(sx + 4);
            int16_t my = floordiv8(sy + 4);

            uint8_t cell = Game_ReadMassBlock(mx, my);
            if (cell != MASS_EMPTY) return 1;
        }
    }
    return 0;
}


void Game_DestroyPiecesDuringRotationSystem() {
    if (!gameState.massGrid.rotating) return;

    int i;
    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        Game_FallingPiece* piece = &gameState.fallingPieces[i];
        if (!piece->active) continue;
        if (Game_TestCollisionWithRotatingMass(piece)) {
            Render_ErasePiece(piece);
            piece->active = 0;
        }
    }
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
            piece->moved = 1;
        }

        // delete if outside the screen and then spawn one more
        if (piece->pos.y > GLOBAL_GRID_H) {
            piece->active = 0;
        }
    }
}

void Game_PiecesSpawnSystem() {
    if (gameState.spawnCpt == 0) {
        Game_SpawnRandomPiece();
        gameState.spawnCpt = SPAWN_PIECE_INTERVAL;
    } else {
        gameState.spawnCpt--;
    }

}

void Game_Init() {
    Game_SetMassBlock(0, 0, MASS_CORE);
    Game_SetMassBlock(1, 0, MASS_SOLID);
    Game_SetMassBlock(-1, 0, MASS_SOLID);
    Game_SetMassBlock(0, 1, MASS_SOLID);
    Game_SetMassBlock(0, -1, MASS_SOLID);

    Game_SetMassBlock(1, 1, MASS_SOLID);
    Game_SetMassBlock(-1, 1, MASS_SOLID);
    Game_SetMassBlock(-1, -1, MASS_SOLID);
    Game_SetMassBlock(-1, 1, MASS_SOLID);
    Game_SetMassBlock(2, 2, MASS_SOLID);
    Game_SetMassBlock(1, 2, MASS_SOLID);
    Game_SetMassBlock(0, 2, MASS_SOLID);
    Game_SetMassBlock(-1, 2, MASS_SOLID);


    Game_UpdateMassAabb();

    Game_SetMassPosition((ivec2){GLOBAL_GRID_W/2, GLOBAL_GRID_H/2});

    // initial render
    Render_FlagMassAsDirty();
}
