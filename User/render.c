#include "render.h"

#include <stdio.h>
#include <string.h>

#include "affichagelcd.h"
#include "game.h"
#include "lcd_api.h"

Render_State renderState = {0};

const Sprite *const SPT_TETROMINOS[] = {
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

void Render_ErasePiece(const Game_FallingPiece *piece) {
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;
    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            if (!shape[ty][tx]) continue;
            int8_t cellX = piece->pos.x + tx;
            int8_t cellY = piece->pos.y + ty;
            if (Game_ReadMassBlock(cellX - gameState.massGrid.pos.x, cellY - gameState.massGrid.pos.y) != MASS_EMPTY)
                continue;

            Render_EraseCell((ivec2){cellX, cellY});
        }
    }
}

void Render_DrawMass() {
    const Game_MassGrid *grid = &gameState.massGrid;
    const Game_MassGrid *prevGrid = &renderState.massState.prevGrid;
    const ivec2 currPos = grid->pos;
    const ivec2 prevPos = prevGrid->pos;
    const uint8_t explosionBlinkStatus = grid->explosionBlinkStatus;
    const uint8_t moved = (currPos.x != prevPos.x) || (currPos.y != prevPos.y);

    // merge the two grid to parkour in the world grid
    int16_t wx0 = (currPos.x < prevPos.x ? currPos.x : prevPos.x) - MASS_GRID_W_HALF;
    int16_t wx1 = (currPos.x > prevPos.x ? currPos.x : prevPos.x) + MASS_GRID_W_HALF;
    int16_t wy0 = (currPos.y < prevPos.y ? currPos.y : prevPos.y) - MASS_GRID_H_HALF;
    int16_t wy1 = (currPos.y > prevPos.y ? currPos.y : prevPos.y) + MASS_GRID_H_HALF;

    // for each cell of the world grid
    int16_t wx, wy;
    for (wx = wx0; wx < wx1; wx++) {
        for (wy = wy0; wy < wy1; wy++) {
            if (wx < 0 || wy < 0 || wx > GLOBAL_GRID_W || wy > GLOBAL_GRID_H) continue;
            // local coords
            int16_t cmx = wx - currPos.x;
            int16_t cmy = wy - currPos.y;
            int16_t pmx = wx - prevPos.x;
            int16_t pmy = wy - prevPos.y;

            uint8_t curr = MASS_EMPTY;
            if (cmx >= -MASS_GRID_W_HALF && cmx < MASS_GRID_W_HALF &&
                cmy >= -MASS_GRID_H_HALF && cmy < MASS_GRID_H_HALF) {
                curr = grid->grid[cmx + MASS_GRID_W_HALF][cmy + MASS_GRID_H_HALF];
            }

            uint8_t prev = MASS_EMPTY;
            if (pmx >= -MASS_GRID_W_HALF && pmx < MASS_GRID_W_HALF &&
                pmy >= -MASS_GRID_H_HALF && pmy < MASS_GRID_H_HALF) {
                prev = prevGrid->grid[pmx + MASS_GRID_W_HALF][pmy + MASS_GRID_H_HALF];
            }

            // if not moved AND curr == prev OR moved and curr == prev, stop here
            if (!moved && curr == prev) continue;
            if (moved && curr == prev) continue;

            ivec2 pixel = cellToPixel((ivec2){wx, wy});
            // if (pixel.x < 0 || pixel.x > LCD_HEIGHT - 8 || pixel.y < 0 || pixel.y > LCD_WIDTH - 8) continue;

            if (curr == MASS_SOLID) {
                Render_BlitSprite(&SPT_GoldCube, pixel.x, pixel.y);
            } else if (curr == MASS_CRITICAL) {
                if (explosionBlinkStatus == 1)
                    Render_BlitSprite(&SPT_GoldCube, pixel.x, pixel.y);
                else
                    Render_BlitSprite(&SPT_SilverCube, pixel.x, pixel.y);
            } else if (curr == MASS_EXPLOSION_BEAM) {
                Lcd_StartRWPacket pkt = {
                    .pos = {pixel.x, pixel.y},
                    .size = {8, 8},
                    .dir = 0
                };
                Lcd_StartReadWriteGRAM(&pkt);
                Lcd_WriteFillGRAM(White, 64);
            } else {
                Render_EraseCell((ivec2){wx, wy});
            }
        }
    }

    memcpy(&renderState.massState.prevGrid, grid, sizeof(Game_MassGrid));
    renderState.massState.prevGrid.pos = currPos;
}

/**
 * Use the scanline methods like a rasterizer
 */
void Render_DrawRotatedMass(int16_t cos_v, int16_t sin_v) {
    // Compute radius of the bounding circle of the mass in pixels
    const aabb massAabb = gameState.massGrid.aabb;
    const uint8_t explosionBlinkStatus = gameState.massGrid.explosionBlinkStatus;
    const int16_t halfExtentCellX = (-massAabb.min.x > massAabb.max.x ? -massAabb.min.x : massAabb.max.x);
    const int16_t halfExtentCellY = (-massAabb.min.y > massAabb.max.y ? -massAabb.min.y : massAabb.max.y);
    const int16_t halfExtentPxX = halfExtentCellX * 8 + 4;
    const int16_t halfExtentPxY = halfExtentCellY * 8 + 4;
    const uint16_t windowRadius = isqrt32(halfExtentPxX * halfExtentPxX + halfExtentPxY * halfExtentPxY) + 1;

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
    if (winY1 > LCD_WIDTH - 1) winY1 = LCD_WIDTH - 1;
    if (winX1 < winX0 || winY1 < winY0) return;

    Lcd_StartRWPacket pkt = {
        .pos = {winX0, winY0},
        .size = {winX1 - winX0 + 1, winY1 - winY0 + 1},
        .dir = 0
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
        for (tx = 0; tx < 4; tx++)
            for (ty = 0; ty < 4; ty++) {
                if (!shape[ty][tx]) continue;
                int16_t gx = p->pos.x + tx;
                int16_t gy = p->pos.y + ty;
                if (gx < 0 || gx >= GLOBAL_GRID_W || gy < 0 || gy >= GLOBAL_GRID_H) continue;
                cacheGlobalGrid[gx][gy] = p->type;
            }
    }

    // for eahc pixel
    int16_t screenX, screenY;
    for (screenY = winY0; screenY <= winY1; screenY++) {
        const int16_t relY = screenY - pivotY;
        // Fixed-point (Q8) accumulator: source coords at the start of this scanline
        int32_t srcAccX = (int32_t) cos_v * (winX0 - pivotX) + (int32_t) sin_v * relY;
        int32_t srcAccY = -(int32_t) sin_v * (winX0 - pivotX) + (int32_t) cos_v * relY;

        for (screenX = winX0; screenX <= winX1; screenX++) {
            // get the pixel coordinates in the source unrotated space
            const int16_t srcPxX = srcAccX / 256;
            const int16_t srcPxY = srcAccY / 256;

            // get the coordinate in the local mass to get the pixel in the gold block sprite
            const int16_t localPxX = srcPxX + 4;
            const int16_t localPxY = srcPxY + 4;
            const int16_t massCellX = floordiv8(localPxX);
            const int16_t massCellY = floordiv8(localPxY);
            const int16_t spriteX = localPxX - massCellX * 8;
            const int16_t spriteY = localPxY - massCellY * 8;

            uint16_t color = Black;
            uint8_t cell = Game_ReadMassBlock(massCellX, massCellY);
            if (cell == MASS_SOLID || cell == MASS_CORE) {
                color = SPT_GoldCube.data[spriteY * 8 + spriteX];
            } else if (cell == MASS_CRITICAL || cell == MASS_CORE_CRITICAL) {
                if (explosionBlinkStatus == 1)
                    color = SPT_GoldCube.data[spriteY * 8 + spriteX];
                else
                    color = SPT_SilverCube.data[spriteY * 8 + spriteX];
            } else if (cell == MASS_EXPLOSION_BEAM) {
                color = White;
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
    Game_MassGrid *g = &gameState.massGrid;

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

void Render_RenderTetromino(const Game_FallingPiece *piece) {
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < MAX_FALLING_PIECES; tx++) {
        for (ty = -1; ty < 4; ty++) {
            ivec2 absPos = {piece->pos.x + tx, piece->pos.y + ty};
            if (absPos.x < 0 || absPos.y < 0 || absPos.x > GLOBAL_GRID_W || absPos.y > GLOBAL_GRID_H-1) continue;
            // dont render out of the screen

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


void Render_PvBar() {
    Lcd_StartRWPacket pkt = {
        .pos = {0, LCD_WIDTH - 2},
        .size = {LCD_HEIGHT, 2},
        .dir = 1
    };
    int pvWidth = LCD_HEIGHT*((gameState.pv)/100.f);
    int emptyWidth = LCD_HEIGHT - pvWidth;

    Lcd_StartReadWriteGRAM(&pkt);
    Lcd_WriteFillGRAM(Green, pvWidth*2);
    Lcd_WriteFillGRAM(Black, emptyWidth*2);
}

#define HUD_BACKGROUND_COLOR RGB565(50, 50, 50)
void Render_RenderHUD() {
    char scoreStr[9];
    char hiScoreStr[9];
    int offsetY = LCD_WIDTH - HUD_HEIGHT_PX;

    sprintf(scoreStr, "%08lu", (uint32_t) gameState.score);
    sprintf(hiScoreStr, "%08lu", (uint32_t) gameState.hiScore);

    LCD_write_english_string(0, offsetY, "SCORE", White, HUD_BACKGROUND_COLOR);
    LCD_write_english_string(40, offsetY, scoreStr, White, HUD_BACKGROUND_COLOR);

    int hx = (LCD_HEIGHT / 2) - 6;
    LCD_write_english_string(hx, offsetY, "HISCORE", White, HUD_BACKGROUND_COLOR);
    LCD_write_english_string(hx + 56 , offsetY, hiScoreStr, White, HUD_BACKGROUND_COLOR);

    Render_PvBar();
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

    // if (gameState.score != renderState.prevScore || gameState.pv != renderState.prevPv || gameState.hiScore != renderState.prevHiScore) {
    //     renderState.prevScore = gameState.score;
    //     renderState.prevPv = gameState.pv;
    //     renderState.prevHiScore = gameState.hiScore;
        Render_RenderHUD();
    // }
}
