#include "lpc17xx_gpio.h"
#include "math.h"
#ifndef ILI_LCD_GENERAL_H_INCLUDED
#define ILI_LCD_GENERAL_H_INCLUDED
/*
 Compatible list:
 ili9320 ili9325 ili9328
 LG4531
*/
/*********************** Hardware specific configuration **********************/

/* 8bit to 16bit LCD Interface

   PINS:
   - EN      = P1.25
   - DIR     = P1.24
   - LE      = P1.23
   - WR      = P0.23
   - CS      = P0.22
   - RD      = P0.21
   - RS      = P0.20
   - DB[0.7] = P2.0...P2.7
   - DB[8.15]= P2.0...P2.7                                                     */

#define PIN_EN		(1 << 25)
#define PIN_DIR		(1 << 24)
#define PIN_LE		(1 << 23)
#define PIN_WR		(1 << 23)
#define PIN_CS      (1 << 22)
#define PIN_RD		(1 << 21)
#define PIN_RS		(1 << 20)

/*------------------------- Speed dependant settings -------------------------*/

/* If processor works on high frequency delay has to be increased, it can be
   increased by factor 2^N by this constant                                   */
#define DELAY_2N    8

/*--------------- Graphic LCD interface hardware definitions -----------------*/

/* Pin EN setting to 0 or 1                                                   */
#define LCD_EN(x)   ((x) ? (LPC_GPIO1->FIOSET = PIN_EN) : (LPC_GPIO1->FIOCLR = PIN_EN));
/* Pin DIR setting to 0 or 1                                                   */
#define LCD_DIR(x)   ((x) ? (LPC_GPIO1->FIOSET = PIN_DIR) : (LPC_GPIO1->FIOCLR = PIN_DIR));
/* Pin LE setting to 0 or 1                                                   */
#define LCD_LE(x)   ((x) ? (LPC_GPIO1->FIOSET = PIN_LE) : (LPC_GPIO1->FIOCLR = PIN_LE));
/* Pin WR setting to 0 or 1                                                   */
#define LCD_WR(x)   ((x) ? (LPC_GPIO0->FIOSET = PIN_WR) : (LPC_GPIO0->FIOCLR = PIN_WR));
/* Pin CS setting to 0 or 1                                                   */
#define LCD_CS(x)   ((x) ? (LPC_GPIO0->FIOSET = PIN_CS) : (LPC_GPIO0->FIOCLR = PIN_CS));
/* Pin RD setting to 0 or 1                                                   */
#define LCD_RD(x)   ((x) ? (LPC_GPIO0->FIOSET = PIN_RD) : (LPC_GPIO0->FIOCLR = PIN_RD));
/* Pin RS setting to 0 or 1                                                   */
#define LCD_RS(x)   ((x) ? (LPC_GPIO0->FIOSET = PIN_RS) : (LPC_GPIO0->FIOCLR = PIN_RS));
/* LCD color */
#define RGB565(R, G, B)  ((uint16_t)( (((R) & 0xF8) << 8) | (((G) & 0xFC) << 3) | (((B) & 0xF8) >> 3) ))
#define White            0xFFFF
#define Black            0x0000
#define Grey             0xF7DE
#define Blue             0x001F
#define Blue2            0x051F
#define Red              0xF800
#define Magenta          0xF81F
#define Green            0x07E0
#define Cyan             0x7FFF
#define Yellow           0xFFE0

/*---------------------- Graphic LCD size definitions ------------------------*/
#define LCD_WIDTH       320                 /* Screen Width (in pixels)           */
#define LCD_HEIGHT      240                 /* Screen Hight (in pixels)           */
#define BPP             16                  /* Bits per pixel                     */
#define BYPP            ((BPP+7)/8)         /* Bytes per pixel                    */


/**
 * Packet definition to prepare registers before drawing/reading
 * pos: position of the first pixel to read/write. Will set the cursor at. Is also the position of the window if size is different of 0, 0
 * size: size of the draw window. If 0, 0, the window is the whole screen and pos is only the position of the cursor
 * dir: direction of the window. If horizontal, the window is drawn/read horizontally, vice versa.
 */
typedef struct {
    ivec2 pos;
    // Window
    ivec2 size;
    uint8_t dir; // 0 = horizontal, 1 = vertical
} Lcd_StartRWPacket;

extern void Lcd_Initializtion(void);
extern void Lcd_SetCursor(unsigned int x,unsigned int y);
extern unsigned int Lcd_getDeviceId(void);
extern void Lcd_StartReadWriteGRAM(const Lcd_StartRWPacket* packet);

/**
 * Write a pixel to the GRAM
 * @param data color of the pixel to write
 */
extern void Lcd_Write(unsigned short data);

/**
 * Write n pixels to the GRAM of the same color
 * @param data color of the pixel to write
 * @param n count of pixels to write
 */
extern void Lcd_WriteFillGRAM(unsigned short data, unsigned int n);

/**
 * Write n pixels to the GRAM with the color of each pixel in the data array
 * @param data Array of n size containing colors to write in orde
 * @param n size of the array
 */
extern void Lcd_WriteBufferGRAM(const unsigned short* data, unsigned int n);

/**
 * Clear the whole screen. Will do a call to Lcd_WriteFillGRAM with the whole screen size and the color to fill
 * @param Color color to fill the screen with
 */
extern void Lcd_Clear(unsigned short Color);

#endif // ILI_LCD_GENERAL_H_INCLUDED
