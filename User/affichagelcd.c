#include "touch/ili_lcd_general.h"
#include "touch/lcd_api.h"
#include "touch/touch_panel.h"

void dessiner_ligne(unsigned int x, unsigned int y, unsigned int l,unsigned int e, char orientation, unsigned short color)
{
	int w,h;

	if (orientation == 'h') {
		w = e;
		h = l;
	} else {
		w = l;
		h = e;
	}

	Lcd_StartRWPacket pkt = {
		.pos = {x, y},
		.size = {w, h},
		.dir = 0
	};
	Lcd_StartReadWriteGRAM(&pkt);
	Lcd_WriteFillGRAM(color, w * h);
}

void dessiner_rect(unsigned int x, unsigned int y, unsigned int lng, unsigned int lrg, unsigned int e, unsigned short plein, unsigned short e_color, unsigned short bg_color)
{
	//dessiner fond
	if(plein==1)
	{
		dessiner_ligne(x,y,lng,lrg,'h',bg_color);
	}
	
	//dessiner bordures
	dessiner_ligne(x,y,lng,e,'h',e_color);
	dessiner_ligne(x+lng-e,y,lrg,e,'v',e_color);
	dessiner_ligne(x,y+lrg-e,lng,e,'h',e_color);
	dessiner_ligne(x,y,lrg,e,'v',e_color);
}

