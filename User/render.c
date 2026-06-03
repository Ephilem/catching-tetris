#include "render.h"

#include <string.h>

#include "affichagelcd.h"
#include "game.h"
#include "ili_lcd_general.h"

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

    if (blink == 2) {
        dessiner_ligne(pixelPos.x, pixelPos.y, 8, 2, 'v', CORE_INDICATOR_COLOR);
        dessiner_ligne(pixelPos.x, pixelPos.y + 6, 8, 2, 'v', CORE_INDICATOR_COLOR);
        dessiner_ligne(pixelPos.x, pixelPos.y, 8, 2, 'h', CORE_INDICATOR_COLOR);
        dessiner_ligne(pixelPos.x + 6, pixelPos.y, 8, 2, 'h', CORE_INDICATOR_COLOR);

        dessiner_ligne(pixelPos.x + 3, pixelPos.y + 3, 2, 2, 'h', CORE_INDICATOR_COLOR);
    } else {
        Render_BlitSprite(&SPT_GoldCube, pixelPos.x, pixelPos.y);
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
            } else if (prev != MASS_EMPTY) {
                Render_EraseCell((ivec2){mx + currPos.x, my + currPos.y});
            }
        }
    }

    memcpy(&renderState.massState.prevGrid.grid, &grid->grid, sizeof(grid->grid));
    renderState.massState.prevGrid.pos = currPos;
}


static __INLINE int floordiv8(int v) {
    return (v >= 0) ? (v / 8) : -((7 - v) / 8);
}

/**
 * Use the scanline methods like a rasterizer
 */
void Render_DrawRotatedMass(int16_t cos_v, int16_t sin_v) {
    const aabb a = gameState.massGrid.aabb;
    const int16_t max_cells_x = (-a.min.x > a.max.x ? -a.min.x : a.max.x);
    const int16_t max_cells_y = (-a.min.y > a.max.y ? -a.min.y : a.max.y);
    const int16_t hx = max_cells_x * 8 + 4;
    const int16_t hy = max_cells_y * 8 + 4;
    const int16_t R  = hx + hy;

    const ivec2 mc = cellToPixel(gameState.massGrid.pos);
    const int16_t centerX = mc.x + 4;
    const int16_t centerY = mc.y + 4;

    int16_t x0 = centerX - R, x1 = centerX + R - 1;
    int16_t y0 = centerY - R, y1 = centerY + R - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > LCD_HEIGHT - 1) x1 = LCD_HEIGHT - 1;
    if (y1 > LCD_WIDTH - 1)  y1 = LCD_WIDTH - 1;
    if (x1 < x0 || y1 < y0) return;

    const int16_t width  = x1 - x0 + 1;
    const int16_t height = y1 - y0 + 1;

    Lcd_StartRWPacket pkt = {
        .pos  = {x0, y0},
        .size = {width, height},
        .dir  = 0
    };
    Lcd_StartReadWriteGRAM(&pkt);

    // for each pixels
    int16_t ax, ay;
    for (ay = y0; ay <= y1; ay++) {
        const int16_t py = ay - centerY;
        for (ax = x0; ax <= x1; ax++) {
            const int16_t px = ax - centerX;

            // ivnerse rotation to found the pixel in the unrotated mass
            const int16_t sx = ( cos_v * px + sin_v * py) / 256;
            const int16_t sy = (-sin_v * px + cos_v * py) / 256;

            // get the cell so we can know what pixel of the sprite only if there is a mass solid here
            const int16_t ix = sx + 4;
            const int16_t iy = sy + 4;
            const int16_t mx = floordiv8(ix);
            const int16_t my = floordiv8(iy);
            const int16_t in_x = ix - mx * 8;
            const int16_t in_y = iy - my * 8;

            uint16_t color = Black;
            uint8_t cell = Game_ReadMassBlock(mx, my);
            if (cell == MASS_SOLID || cell == MASS_CORE) {
                color = SPT_GoldCube.data[in_y * 8 + in_x];
            }

            Lcd_Write(color);
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

    for (i = 0; i < 4; i++) {
        if (gameState.fallingPieces[i].active) {
            Render_RenderTetromino(&gameState.fallingPieces[i]);
        }
    }

    if (gameState.massGrid.rotating) {
        // pendant la rotation, on dessine la masse tournee (auto-effacante)
        Render_StepAndDrawRotation();
    } else {
        // if (renderState.massState.flag_massChanged == 1) {
            // renderState.massState.flag_massChanged = 0;
            Render_DrawMass();
        // }
    }

    // Always render because it has an animation
    Render_DrawMassCore();
}
