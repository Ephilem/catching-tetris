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

    for (mx = -MASS_GRID_W_HALF; mx <= MASS_GRID_W_HALF; mx++) {
        for (my = -MASS_GRID_H_HALF; my <= MASS_GRID_H_HALF; my++) {
            uint8_t curr = MASS_EMPTY;
            curr = grid->grid[mx + MASS_GRID_W_HALF][my + MASS_GRID_H_HALF];

            // Trying to get the content of the previous case
            int8_t pmx = mx + dx;
            int8_t pmy = my + dy;
            uint8_t prev = MASS_EMPTY;
            if (pmx >= -MASS_GRID_W_HALF && pmx <= MASS_GRID_W_HALF && pmy >= -MASS_GRID_H_HALF && pmy <= MASS_GRID_H_HALF) {
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

void Render_RenderTetromino(const Game_FallingPiece* piece) {
    const uint8_t (*shape)[4] = TETROMINOS[piece->type][piece->rotation];
    int8_t tx, ty;

    for (tx = 0; tx < 4; tx++) {
        for (ty = 0; ty < 4; ty++) {
            ivec2 absPos = {piece->pos.x + tx, piece->pos.y + ty};
            if (absPos.x < 0 || absPos.y < 0) continue; // dont render out of the screen

            // get prev (dy = -1) value
            int8_t pmy = ty - 1;
            uint8_t prevEmpty = 1;
            if (pmy >= 0) {
                prevEmpty = !shape[pmy][tx];
            }

            if (shape[ty][tx])
                Render_BlitSprite(SPT_TETROMINOS[piece->type], absPos.x * 8, absPos.y * 8);
            else if (prevEmpty)
                Render_EraseCell(absPos);
        }
    }
}

void Render_Render() {
    int i = 0;
    if (renderState.massState.flag_massChanged == 1) {
        renderState.massState.flag_massChanged = 0;
        Render_DrawMass();
    }

    // Always render because it has an animation
    Render_DrawMassCore();
    for (i = 0; i < 4; i++) {
        if (gameState.fallingPieces[i].active) {
            Render_RenderTetromino(&gameState.fallingPieces[i]);
        }
    }
}
