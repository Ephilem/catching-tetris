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
} Render_State;

extern Render_State renderState;

void Render_BlitSprite(const Sprite* sprite, int x, int y);

void Render_FlagMassAsDirty();
void Render_DrawMass();
void Render_DrawMassCore();

void Render_Render();

#endif
