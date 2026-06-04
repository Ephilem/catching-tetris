#include "render.h"

#include <string.h>

#include "affichagelcd.h"
#include "game.h"

Render_State renderState = {0};

const Sprite* const SPT_TETROMINOS[] = {
    &SPT_CyanCube, &SPT_YellowCube, &SPT_MagentaCube,
    &SPT_GreenCube, &SPT_RedCube, &SPT_BlueCube, &SPT_OrangeCube
};

void Render_FlagMassAsDirty() {
    renderState.massState.flag_massChanged = 1;
}

void Render_BlitSprite(const Sprite *sprite, int x, int y) {
    Lcd_StartRWPacket pkt = {
        .pos = {x, y},
        .size = {sprite->width, sprite->height},
        .dir = 0
    };
    Lcd_StartReadWriteGRAM(&pkt);
    Lcd_WriteBufferGRAM(sprite->data, sprite->width * sprite->height);
}

__INLINE ivec2 cellToPixel(ivec2 cell) {
    return (ivec2){cell.x * 8, cell.y * 8};
}

#define CORE_INDICATOR_COLOR RGB565(0, 162, 73)

void Render_DrawMassCore() {
    ivec2 corePos = gameState.massGrid.pos;
    ivec2 pixelPos = cellToPixel(corePos);

    uint8_t blink = renderState.massState.anim_massCoreBlink;

    uint8_t coreState = Game_ReadMassBlock(0, 0);

    if (blink == 2) {
        dessiner_ligne(pixelPos.x, pixelPos.y, 8, 2, 'v', CORE_INDICATOR_COLOR);
        dessiner_ligne(pixelPos.x, pixelPos.y + 6, 8, 2, 'v', CORE_INDICATOR_COLOR);
        dessiner_ligne(pixelPos.x, pixelPos.y, 8, 2, 'h', CORE_INDICATOR_COLOR);
        dessiner_ligne(pixelPos.x + 6, pixelPos.y, 8, 2, 'h', CORE_INDICATOR_COLOR);

        dessiner_ligne(pixelPos.x + 3, pixelPos.y + 3, 2, 2, 'h', CORE_INDICATOR_COLOR);
    } else {
        if (coreState == MASS_CORE_CRITICAL) {
            Render_BlitSprite(&SPT_SilverCube, pixelPos.x, pixelPos.y);
        } else {
            Render_BlitSprite(&SPT_GoldCube, pixelPos.x, pixelPos.y);
        }
    }

    blink++;
    if (blink > 3) {
        blink = 0;
    }
    renderState.massState.anim_massCoreBlink = blink;
}

void Render_EraseCell(ivec2 cell) {
    ivec2 pixelPos = cellToPixel(cell);
    Lcd_StartRWPacket pkt = {
        .pos = {pixelPos.x, pixelPos.y},
        .size = {8, 8},
        .dir = 0
    };
    Lcd_StartReadWriteGRAM(&pkt);
    Lcd_WriteFillGRAM(Black, 64);
}

void Render_ErasePiece(const Game_FallingPiece* piece) {
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;
    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;
            int8_t cellX = piece->pos.x + tx;
            int8_t cellY = piece->pos.y + ty;
            if (Game_ReadMassBlock(cellX - gameState.massGrid.pos.x, cellY - gameState.massGrid.pos.y) != MASS_EMPTY) continue;

            Render_EraseCell((ivec2){cellX, cellY});
        }
    }
}

void Render_DrawMass() {
    const Game_MassGrid *grid = &gameState.massGrid;
    const Game_MassGrid *prevGrid = &renderState.massState.prevGrid;
    const ivec2 currPos = grid->pos;
    const ivec2 prevPos = prevGrid->pos;
    const int8_t dx = currPos.x - prevPos.x;
    const int8_t dy = currPos.y - prevPos.y;
    const uint8_t moved = dx != 0 || dy != 0;
    int8_t mx, my;

    for (mx = -MASS_GRID_W_HALF; mx < MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my < MASS_GRID_H_HALF; my++) {
            uint8_t curr = MASS_EMPTY;
            curr = grid->grid[mx + MASS_GRID_W_HALF][my + MASS_GRID_H_HALF];

            // Trying to get the content of the previous case
            int8_t pmx = mx + dx;
            int8_t pmy = my + dy;
            uint8_t prev = MASS_EMPTY;
            if (pmx >= -MASS_GRID_W_HALF && pmx < MASS_GRID_W_HALF && pmy >= -MASS_GRID_H_HALF && pmy < MASS_GRID_H_HALF) {
                prev = prevGrid->grid[pmx + MASS_GRID_W_HALF][pmy + MASS_GRID_H_HALF];
            }

            if (!moved && curr == prev) continue;

            ivec2 pixel = cellToPixel((ivec2){mx + currPos.x, my + currPos.y});
            if (curr == MASS_SOLID) {
                Render_BlitSprite(&SPT_GoldCube, pixel.x, pixel.y);
            } else if (curr == MASS_CRITICAL) {
                Render_BlitSprite(&SPT_SilverCube, pixel.x, pixel.y);
            } else if (prev != MASS_EMPTY) {
                Render_EraseCell((ivec2){mx + currPos.x, my + currPos.y});
            }
        }
    }

    memcpy(&renderState.massState.prevGrid.grid, &grid->grid, sizeof(grid->grid));
    renderState.massState.prevGrid.pos = currPos;
}

/**
 * Use the scanline methods like a rasterizer
 */
void Render_DrawRotatedMass(int16_t cos_v, int16_t sin_v) {
    // Compute radius of the bounding circle of the mass in pixels
    const aabb massAabb = gameState.massGrid.aabb;
    const int16_t halfExtentCellX = (-massAabb.min.x > massAabb.max.x ? -massAabb.min.x : massAabb.max.x);
    const int16_t halfExtentCellY = (-massAabb.min.y > massAabb.max.y ? -massAabb.min.y : massAabb.max.y);
    const int16_t halfExtentPxX = halfExtentCellX * 8 + 4;
    const int16_t halfExtentPxY = halfExtentCellY * 8 + 4;
    const uint16_t windowRadius = isqrt32(halfExtentPxX*halfExtentPxX + halfExtentPxY*halfExtentPxY) + 1;

    // Pivot point: center of the mass core cell in pixels
    const ivec2 massCellPx = cellToPixel(gameState.massGrid.pos);
    const int16_t pivotX = massCellPx.x + 4;
    const int16_t pivotY = massCellPx.y + 4;

    // Screen-space window around the pivot, clamped to screen bounds
    int16_t winX0 = pivotX - windowRadius, winX1 = pivotX + windowRadius - 1;
    int16_t winY0 = pivotY - windowRadius, winY1 = pivotY + windowRadius - 1;
    if (winX0 < 0) winX0 = 0;
    if (winY0 < 0) winY0 = 0;
    if (winX1 > LCD_HEIGHT - 1) winX1 = LCD_HEIGHT - 1;
    if (winY1 > LCD_WIDTH - 1)  winY1 = LCD_WIDTH - 1;
    if (winX1 < winX0 || winY1 < winY0) return;

    Lcd_StartRWPacket pkt = {
        .pos  = {winX0, winY0},
        .size = {winX1 - winX0 + 1, winY1 - winY0 + 1},
        .dir  = 0
    };
    Lcd_StartReadWriteGRAM(&pkt);

    // Prepare a cache where are tetrominos on the grid
    static uint8_t cacheGlobalGrid[GLOBAL_GRID_W][GLOBAL_GRID_H];
    memset(cacheGlobalGrid, 0xFF, sizeof(cacheGlobalGrid));

    int i;
    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        const Game_FallingPiece *p = &gameState.fallingPieces[i];
        if (!p->active) continue;
        const uint8_t (*shape)[4] = TETROMINOS[p->type][p->rotation];

        int8_t tx, ty;
        for (tx = 0; tx < 4; tx++) for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;
            int16_t gx = p->pos.x + tx;
            int16_t gy = p->pos.y + ty;
            if (gx < 0 || gx >= GLOBAL_GRID_W || gy < 0 || gy >= GLOBAL_GRID_H) continue;
            cacheGlobalGrid[gx][gy] = p->type;
        }
    }

    // For each screen pixel in the window, inverse-rotate to find the source pixel in the unrotated mass
    int16_t screenX, screenY;
    for (screenY = winY0; screenY <= winY1; screenY++) {
        const int16_t relY = screenY - pivotY;
        // Fixed-point (Q8) accumulator: source coords at the start of this scanline
        int32_t srcAccX =  (int32_t)cos_v * (winX0 - pivotX) + (int32_t)sin_v * relY;
        int32_t srcAccY = -(int32_t)sin_v * (winX0 - pivotX) + (int32_t)cos_v * relY;

        // int16_t lastCellX = -1000, lastCellY = screenY * 8;
        // uint8_t cachedPieceType = 0xFF;
        for (screenX = winX0; screenX <= winX1; screenX++) {
            // Source pixel in the unrotated mass (in pixels, relative to the mass core center)
            const int16_t srcPxX = srcAccX / 256;
            const int16_t srcPxY = srcAccY / 256;

            // Shift by 4 to center within the 8px cell, then find the cell and the pixel within it
            const int16_t localPxX = srcPxX + 4;
            const int16_t localPxY = srcPxY + 4;
            const int16_t massCellX   = floordiv8(localPxX);
            const int16_t massCellY   = floordiv8(localPxY);
            const int16_t spriteX = localPxX - massCellX * 8;
            const int16_t spriteY = localPxY - massCellY * 8;

            uint16_t color = Black;
            uint8_t cell = Game_ReadMassBlock(massCellX, massCellY);
            if (cell == MASS_SOLID || cell == MASS_CORE) {
                color = SPT_GoldCube.data[spriteY * 8 + spriteX];
            } else if (cell == MASS_CRITICAL || cell == MASS_CORE_CRITICAL) {
                color = SPT_SilverCube.data[spriteY * 8 + spriteX];
            } else {
                // draw piece if there is one at this position.
                int16_t globalCellX = screenX / 8;
                int16_t globalCellY = screenY / 8;
                uint8_t cachedPieceType = cacheGlobalGrid[globalCellX][globalCellY];
                if (cachedPieceType != 0xFF) {
                    color = SPT_TETROMINOS[cachedPieceType]->data[(screenY & 7) * 8 + (screenX & 7)];
                }
            }

            Lcd_Write(color);

            srcAccX += cos_v;
            srcAccY -= sin_v;
        }
    }
}

void Render_StepAndDrawRotation() {
    Game_MassGrid* g = &gameState.massGrid;

    if (g->rotCur < g->rotTarget) {
        g->rotCur += ROT_ANIM_SPEED;
        if (g->rotCur > g->rotTarget) g->rotCur = g->rotTarget;
    } else if (g->rotCur > g->rotTarget) {
        g->rotCur -= ROT_ANIM_SPEED;
        if (g->rotCur < g->rotTarget) g->rotCur = g->rotTarget;
    }

    const uint8_t finished = (g->rotCur == g->rotTarget);
    if (finished) {
        const int quarters = g->rotTarget / ROT_QUARTER_STEPS;
        if (quarters != 0) Game_RotateMassQuarter(quarters);
        g->rotCur = 0;
        g->rotTarget = 0;
    }

    Render_DrawRotatedMass(cos_q8(g->rotCur), sin_q8(g->rotCur));

    if (finished) {
        memcpy(&renderState.massState.prevGrid.grid, &g->grid, sizeof(g->grid));
        renderState.massState.prevGrid.pos = g->pos;
        g->rotating = 0;
    }
}

void Render_RenderTetromino(const Game_FallingPiece* piece) {
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < 4; tx++) {
        for (ty = -1; ty < 4; ty++) {
            ivec2 absPos = {piece->pos.x + tx, piece->pos.y + ty};
            if (absPos.x < 0 || absPos.y < 0 || absPos.x > GLOBAL_GRID_W || absPos.y > GLOBAL_GRID_H) continue; // dont render out of the screen

            // get prev (dy = -1) value
            int8_t pmy = ty - 1;
            uint8_t prevEmpty = 1;
            if (pmy >= 0) {
                prevEmpty = !shape[pmy][tx];
            }

            // skip if there is a mass cell here
            int8_t mx = absPos.x - gameState.massGrid.pos.x;
            int8_t my = absPos.y - gameState.massGrid.pos.y;
            if (Game_ReadMassBlock(mx, my) != MASS_EMPTY) {
                continue;
            }

            if (ty >= 0 && shape[ty][tx])
                Render_BlitSprite(SPT_TETROMINOS[piece->type], absPos.x * 8, absPos.y * 8);
            else if (prevEmpty)
                Render_EraseCell(absPos);
        }
    }
}

void Render_Render() {
    int i = 0;

    if (gameState.massGrid.rotating) {
        Render_StepAndDrawRotation();
    } else {
        if (renderState.massState.flag_massChanged == 1) {
            renderState.massState.flag_massChanged = 0;
            Render_DrawMass();
        }
    }

    for (i = 0; i < MAX_FALLING_PIECES; i++) {
        if (gameState.fallingPieces[i].active && gameState.fallingPieces[i].moved) {
            gameState.fallingPieces[i].moved = 0;
            Render_RenderTetromino(&gameState.fallingPieces[i]);
        }
    }

    // Always render because it has an animation
    Render_DrawMassCore();
}
