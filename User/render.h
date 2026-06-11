#ifndef RENDER_H
#define RENDER_H

#include "sprites_data.h"
#include "game.h"


typedef struct {
    uint8_t flag_massChanged;
    uint8_t anim_massCoreBlink;
    Game_MassGrid prevGrid;
} Render_MassState;

typedef struct {
    Render_MassState massState;
    uint32_t prevScore;
    uint8_t prevPv;
    uint32_t prevHiScore;
} Render_State;

extern Render_State renderState;

void Render_BlitSprite(const Sprite* sprite, int x, int y);

void Render_FlagMassAsDirty();
void Render_EraseCell(ivec2 cell);
void Render_DrawMass();
void Render_DrawMassCore();
void Render_DrawRotatedMass(int16_t cos_v, int16_t sin_v);
void Render_StepAndDrawRotation();
void Render_ErasePiece(const Game_FallingPiece* piece);

void Render_RenderTetromino(const Game_FallingPiece* piece);

void Render_RenderHUD();
void Render_Render();

#endif
