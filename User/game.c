#include "game.h"

#include <string.h>

#include "memory.h"
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

void Game_CheckLevelUp() {
    uint8_t newLvl = 1;
    uint8_t i;
    for (i = 0; i < NUM_BASE_LEVELS; i++) {
        if (gameState.score >= LEVELS[i].scoreThreshold)
            newLvl = i + 1;
        else
            break;
    }
    if (newLvl == NUM_BASE_LEVELS) {
        uint32_t extra = gameState.score - LEVELS[NUM_BASE_LEVELS - 1].scoreThreshold;
        newLvl += (uint8_t)(extra / 20000);
        if (newLvl > 99) newLvl = 99; // max = 99
    }
    gameState.lvl = newLvl;
}

uint16_t Game_GetGravityTicks() {
    uint16_t ms;
    if (gameState.lvl <= NUM_BASE_LEVELS) {
        ms = LEVELS[gameState.lvl - 1].fallPeriodMs;
    } else {
        // dynamicly update the gravity tick (min=100)
        int extra = gameState.lvl - NUM_BASE_LEVELS;
        int val = 240 - extra * 20;
        ms = (val < 100) ? 100 : (uint16_t)val;
    }
    return ms / 10; // because 1 tick is 10ms
}

uint16_t Game_GetSpawnTicks() {
    uint16_t ms;
    if (gameState.lvl <= NUM_BASE_LEVELS) {
        ms = LEVELS[gameState.lvl - 1].spawnPeriodMs;
    } else {
        int extra = gameState.lvl - NUM_BASE_LEVELS;
        int val = 1000 - extra * 50;
        ms = (val < 600) ? 600 : (uint16_t)val;
    }
    return ms / 10; // 1 tick = 10 ms
}

void Game_IncrementScore(uint32_t amount) {
    if (gameState.score + amount > MAX_SCORE) return;

    gameState.score += amount;
    if (gameState.score > gameState.hiScore) {
        gameState.hiScore = gameState.score;
        Memory_Write(MEMORY_HI_SCORE_ADDR, &gameState.hiScore, sizeof(gameState.hiScore));
    }
    Game_CheckLevelUp();
}

void Game_DecrementPV(uint8_t amount) {
    if (amount >= gameState.pv) {
        gameState.pv = 0;
        gameState.gameOver = 1;
    } else {
        gameState.pv -= amount;
    }
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

aabb Game_CalculateMassAabb(uint8_t withBeam) {
    // calculate aabb
    int8_t mx, my;
    aabb box = {127, 127, -127, -127};

    // TODO meilleur façon de faire surement
    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
            uint8_t v = gameState.massGrid.grid[mx+MASS_GRID_W_HALF][my+MASS_GRID_H_HALF];
            if (v == MASS_EMPTY || (v == MASS_EXPLOSION_BEAM && !withBeam)) continue;
            if (mx < box.min.x) box.min.x = mx;
            if (mx > box.max.x) box.max.x = mx;
            if (my < box.min.y) box.min.y = my;
            if (my > box.max.y) box.max.y = my;
        }
    }

    return box;
}

void Game_UpdateMassAabb() {
    gameState.massGrid.aabb = Game_CalculateMassAabb(1);
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

/**
 * After a destruction, there can be some block non connected to the
 * core. We need to bring back tick by tick those blocks to the core to avoid unlogical situation where a block is floating in the air without support
 * Bring back closer by one loose blocks
 * @return the number of blocks brought back
 */
uint8_t Game_BringBackLooseCell() {
    // to not surcharge the pile
    static uint8_t connected[MASS_GRID_W][MASS_GRID_H];
    static ivec2   bfsQueue[MASS_GRID_W * MASS_GRID_H];
    static ivec2   loose   [MASS_GRID_W * MASS_GRID_H];

    memset(connected, 0, sizeof(connected));

    static const int8_t dx8[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static const int8_t dy8[8] = {-1, 0, 1,-1, 1,-1, 0, 1};

    int  head = 0, tail = 0;
    // we know that the core is 0 0
    int8_t coreX = 0, coreY = 0;
    connected[0][0] = 1;
    bfsQueue[tail++] = (ivec2){0, 0};

    // propagate connected
    while (head < tail) {
        ivec2 cur = bfsQueue[head++];
        int d;
        for (d = 0; d < 8; d++) {
            int8_t nx = cur.x + dx8[d];
            int8_t ny = cur.y + dy8[d];
            int ix = nx + MASS_GRID_W_HALF;
            int iy = ny + MASS_GRID_H_HALF;

            // bounds check with one comparaison
            if ((unsigned)ix >= MASS_GRID_W || (unsigned)iy >= MASS_GRID_H) continue;
            if (connected[ix][iy]) continue;

            uint8_t v = Game_ReadMassBlock(nx, ny);
            if (v == MASS_EMPTY || v == MASS_EXPLOSION_BEAM) continue;

            connected[ix][iy] = 1;
            bfsQueue[tail++] = (ivec2){nx, ny};
        }
    }


    //  collect loose blocks
    int looseCount = 0;
    int8_t mx, my;
    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
            int ix = mx + MASS_GRID_W_HALF;
            int iy = my + MASS_GRID_H_HALF;
            if (connected[ix][iy]) continue;

            uint8_t v = Game_ReadMassBlock(mx, my);
            if (v == MASS_EMPTY || v == MASS_EXPLOSION_BEAM) continue;

            loose[looseCount++] = (ivec2){mx, my};
        }
    }


    // move all lose blocks to the core
    int i;
    for (i = 0; i < looseCount; i++) {
        mx = loose[i].x;
        my = loose[i].y;

        uint8_t v = Game_ReadMassBlock(mx, my);
        if (v == MASS_EMPTY) continue;

        int8_t stepX = (coreX > mx) ? 1 : (coreX < mx) ? -1 : 0;
        int8_t stepY = (coreY > my) ? 1 : (coreY < my) ? -1 : 0;

        int8_t nx = mx + stepX;
        int8_t ny = my + stepY;

        if (Game_ReadMassBlock(nx, ny) == MASS_EMPTY) {
            Game_SetMassBlock(nx, ny, v);
            Game_SetMassBlock(mx, my, MASS_EMPTY);
        }
        // sinon : bloqué ce tick, réessaiera au prochain
    }

    Game_UpdateMassAabb();
    Render_FlagMassAsDirty();
    return looseCount;
}

void Game_SetMassPosition(ivec2 pos) {
    // clamp pose with aabb
    ivec2 currPos = gameState.massGrid.pos;
    aabb box = Game_CalculateMassAabb(0);
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

        uint8_t collisionResult = Game_TestCollisionWithMass(piece, (ivec2){0,0});
        if (collisionResult == COLLISION_SOLID) {
            Render_ErasePiece(piece);

            // before fuse, we need to move the piece in the direction of the movement
            piece->pos = (ivec2){
                piece->pos.x + delta.x,
                piece->pos.y + delta.y
            };
            Game_FusePiece(piece);
            break;
        } else if (collisionResult == COLLISION_BEAM) {
            piece->active = 0;
            Render_ErasePiece(piece);
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
            uint8_t v = Game_ReadMassBlock(mx, my);
            if (v == MASS_EMPTY || v == MASS_EXPLOSION_BEAM) continue;

            // check 4x4
            int dx, dy;
            uint8_t found4x4 = 1;
            for (dx = 0; dx < 4 && found4x4; dx++) {
                for (dy = 0; dy < 4 && found4x4; dy++) {
                    uint8_t v2 = Game_ReadMassBlock(mx + dx, my + dy);
                    if (v2 == MASS_EMPTY || v2 == MASS_EXPLOSION_BEAM) {
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
                        Game_IncrementScore(25 * gameState.lvl);
                    }
                }

                // start explosion
                if (gameState.massGrid.explosionTimer == -1) {
                    Game_IncrementScore(100 * gameState.lvl);
                    gameState.massGrid.explosionTimer = START_EXPLOSION_TIMER;
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
    if (js->k1 == BTN_PRESSED) {
        gameState.massGrid.rotTarget -= ROT_QUARTER_STEPS;
        gameState.massGrid.rotating = 1;
    }
    if (js->k2 == BTN_PRESSED) {
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
            uint8_t v = Game_ReadMassBlock(cellX - gameState.massGrid.pos.x, cellY - gameState.massGrid.pos.y);
            if (v == MASS_EXPLOSION_BEAM) {
                return COLLISION_BEAM;
            } else if (v != MASS_EMPTY) {
                return COLLISION_SOLID;
            }
        }
    }

    return COLLISION_NONE;
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

    Game_IncrementScore(5 * gameState.lvl);
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
            Game_DecrementPV(10);
        }
    }
}

void Game_Explode() {
    // saerch for critical blocks
    int mx, my;
    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
            uint8_t v = Game_ReadMassBlock(mx, my);
            if (v == MASS_EMPTY) continue;

            if (v == MASS_CRITICAL) {
                int bx, by;
                for (bx = -MASS_GRID_W_HALF; bx < MASS_GRID_W_HALF; bx++) {
                    uint8_t v2 = Game_ReadMassBlock(bx, my);
                    if (v2 != MASS_CORE && v2 != MASS_CORE_CRITICAL) {
                        Game_SetMassBlock(bx, my, MASS_EXPLOSION_BEAM);
                    }
                }
                for (by = -MASS_GRID_H_HALF; by < MASS_GRID_H_HALF; by++) {
                    uint8_t v2 = Game_ReadMassBlock(mx, by);
                    if (v2 != MASS_CORE && v2 != MASS_CORE_CRITICAL) {
                        Game_SetMassBlock(mx, by, MASS_EXPLOSION_BEAM);
                    }
                }
                Game_IncrementScore(50 * gameState.lvl);
            } else if (v == MASS_CORE_CRITICAL) {
                Game_IncrementScore(50 * gameState.lvl);
                Game_SetMassBlock(mx, my, MASS_CORE);
            }
        }
    }
    Render_FlagMassAsDirty();
    Game_UpdateMassAabb();
}

void Game_ClearExplosionBeam() {
    int mx, my;
    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
            if (Game_ReadMassBlock(mx, my) == MASS_EXPLOSION_BEAM) {
                Game_SetMassBlock(mx, my, MASS_EMPTY);
            }
        }
    }

    Render_FlagMassAsDirty();
    Game_UpdateMassAabb();
    gameState.massGrid.needBringBackLooseBlock = 1;
}

void Game_ExplosionSystemTick() {
    volatile int32_t explosionTimer = gameState.massGrid.explosionTimer;

    if (explosionTimer != -1) {
        explosionTimer--;

        if (explosionTimer == MASS_EXPLOSION_BEAM_DURATION || gameState.joystickState.k6 == BTN_PRESSED) {
            explosionTimer = MASS_EXPLOSION_BEAM_DURATION;
            Game_Explode();
        } else if (explosionTimer == 0) {
            explosionTimer = -1;
            Game_ClearExplosionBeam();
        }
        gameState.massGrid.explosionTimer = explosionTimer;
    }

    gameState.massGrid.explosionTimer = explosionTimer;
}

void Game_BringBackLooseBlockSystemTick() {
    if (gameState.massGrid.needBringBackLooseBlock) {
        gameState.massGrid.needBringBackLooseBlock = 0;
        if (Game_BringBackLooseCell() > 0) {
            gameState.massGrid.cpt_bringBackLooseBlock = 10;
        }
    }

    if (gameState.massGrid.cpt_bringBackLooseBlock > 0) {
        gameState.massGrid.cpt_bringBackLooseBlock--;
        if (gameState.massGrid.cpt_bringBackLooseBlock == 0) {
            gameState.massGrid.needBringBackLooseBlock = 1;
        }
    }
}

void Game_ApplyGravity() {
    int i;

    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        Game_FallingPiece* piece = &gameState.fallingPieces[i];
        if (!piece->active) continue;

        ivec2 nextHop = {0, 1};
        uint8_t collisionResult = Game_TestCollisionWithMass(piece, nextHop);
        if (collisionResult == COLLISION_SOLID) {
            Game_FusePiece(piece);
        } else if (collisionResult == COLLISION_BEAM) {
            Render_ErasePiece(piece);
            piece->active = 0;
        } else {
            piece->pos.y += 1;
            piece->moved = 1;
        }

        // delete if outside the screen
        if (piece->pos.y > GLOBAL_GRID_H) {
            piece->active = 0;
            Game_DecrementPV(5);
        }
    }
}

void Game_PiecesSpawnSystem() {
    if (gameState.spawnCpt == 0) {
        Game_SpawnRandomPiece();
        gameState.spawnCpt = Game_GetSpawnTicks();
    } else {
        gameState.spawnCpt--;
    }

}

void Game_Init() {
    memset(&gameState, 0, sizeof(gameState));
    memset(&renderState, 0, sizeof(renderState));
    Lcd_Clear(Black);
    Game_SetMassBlock(0, 0, MASS_CORE);
    Game_SetMassBlock(1, 0, MASS_SOLID);
    Game_SetMassBlock(-1, 0, MASS_SOLID);
    Game_SetMassBlock(0, 1, MASS_SOLID);
    Game_SetMassBlock(0, -1, MASS_SOLID);

    /*
    Game_SetMassBlock(2, -2, MASS_SOLID);
    Game_SetMassBlock(1, -2, MASS_SOLID);
    Game_SetMassBlock(0, -2, MASS_SOLID);
    Game_SetMassBlock(-1, -2, MASS_SOLID);
    Game_SetMassBlock(2, -1, MASS_SOLID);
    Game_SetMassBlock(1, -1, MASS_SOLID);
    Game_SetMassBlock(0, -1, MASS_SOLID);
    Game_SetMassBlock(-1, -1, MASS_SOLID);

    Game_SetMassBlock(2, 1, MASS_SOLID);
    Game_SetMassBlock(1, 1, MASS_SOLID);
    Game_SetMassBlock(0, 1, MASS_SOLID);
    Game_SetMassBlock(-1, 1, MASS_SOLID);*/


    Game_UpdateMassAabb();

    Game_SetMassPosition((ivec2){GLOBAL_GRID_W/2, GLOBAL_GRID_H/2});

    // initial render
    Render_FlagMassAsDirty();
    gameState.massGrid.explosionTimer = -1;
    gameState.pv = 100;
    Memory_Read(MEMORY_HI_SCORE_ADDR, &gameState.hiScore, sizeof(gameState.hiScore));

    // security for corrupted memory
    if (gameState.hiScore > MAX_SCORE) {
        gameState.hiScore = 0;
    }

    gameState.lvl = 1;
    // Render_RenderHUD();
}
