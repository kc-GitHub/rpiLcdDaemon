//
// Filename:    lcd.c
// Description: LCD graphic routines
//    
// Open Source Licensing 
//
// This program is free software: you can redistribute it and/or modify it under the terms of 
// the GNU General Public License as published by the Free Software Foundation, either version 3 
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.  
// If not, see <http://www.gnu.org/licenses/>.
//                       
// Author:      Martin Steppuhn, www.emsystech.de
// History:     05.11.2012 Initial version V0.9.0
//
//              Dirk Hoffmann <hoffmann@vmd-jena.de>
//              25.05.2013 Some changes for use with the rpiLcdDaemon
//              02.09.2014 switch for rotate the display by 180 degree
//--------------------------------------------------------------------------------------------------

//=== Includes =====================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "std_c.h"
#include "lcd.h"
#include "rpiHardware.h"

//=== Local constants  =============================================================================
#include "../includes/font_terminal_6x8.inc"
#include "../includes/font_terminal_12x16.inc"
#include "../includes/font_fixedsys_8x15.inc"
#include "../includes/font_lucida_10x16.inc"

//=== Local variables ==============================================================================
uint8	framebuffer[LCD_WIDTH][LCD_HEIGHT/8];

uint8	PenColor;
uint8	FillColor;
uint8	FontNumber;
uint8	LCD_xOffset;


/**
 * Set pen color
 *
 * Parameter:	0=White 1=Black
 * Return:		void
 */
void LCD_SetPenColor(uint8 c) {
	PenColor = c;
}

uint8 LCD_GetPenColor() {
	return PenColor;
}

/**
 * Set fill color
 *
 * Parameter:	-1=transparent, 0=white, 1=black
 * Return:		void
 */
void LCD_SetFillColor(int8 c) {
	FillColor = c;
}

/**
 * Set font
 *
 * Parameter:	0..3 for Fonts
 * Return:		void
 */
void LCD_SetFont(uint8 f) {
	FontNumber = f;
}

/**
 * return current font number
 *
 * Parameter:	void
 * Return:		int
 */
int LCD_GetFont(void) {
	return FontNumber;
}

/**
 * Clear Screen
 *
 * Parameter:	void
 * Return:		void
 *
 */
void LCD_ClearScreen(void) {
	uint16	x, y;
	
	for(y=0;y<(LCD_HEIGHT/8);y++) {
		for(x=0;x<LCD_WIDTH;x++)  framebuffer[x][y] = 0;
	}
	framebufferShouldRefresh = 1;
}

/**
 * Set single pixel at x,x
 *
 * Parameter:	x,y position, color
 * Return:		void
 */
void LCD_PutPixel(uint8 x,uint8 y,uint8 color) {
	if((x < LCD_WIDTH) && (y < LCD_HEIGHT)) {
		if(color)	framebuffer[x][y>>3] |=   (1<<(y & 7));
			else	framebuffer[x][y>>3] &=  ~(1<<(y & 7));
	}
	framebufferShouldRefresh = 1;
}

/**
 * Draw line from xy to xy
 *
 * Parameter:	Start and endpoint
 * Return:		void
 */
void LCD_DrawLine(uint8 x0,uint8 y0,uint8 x1,uint8 y1) {
	int16  dx,sx,dy,sy,err,e2;  // for Bresenham
	int8	i;
	
	if(y0 == y1) {			// horizontale Linie
		if(x0 > x1) { i=x0; x0=x1; x1=i; } 	// swap direction
		while (x0 <= x1)	LCD_PutPixel(x0++,y0,PenColor);

	} else if(x0 == x1)	{	// vertikale Linie
		if(y0 > y1) {i=y0; y0=x1; y1=i;}	// swap direction
		while (y0 <= y1)	LCD_PutPixel(x0,y0++,PenColor);

	} else {		// Bresenham Algorithmus
		dx = abs(x1-x0);
		sx = x0<x1 ? 1 : -1;
		dy = -abs(y1-y0);
		sy = y0<y1 ? 1 : -1; 
		err = dx+dy; 
 		for(;;) {
			LCD_PutPixel(x0,y0,PenColor);
			if (x0==x1 && y0==y1) break;
			e2 = 2*err;
			if (e2 > dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
			if (e2 < dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
		}
	}
}

/**
 * Draw circle with centerpont an radius linewidth=1 pixel, no fill
 *
 * Parameter:	xy centerpoint and radius
 * Return:		void
 */
void LCD_DrawCircle(uint8 x0,uint8 y0,uint8 radius) {
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	LCD_PutPixel(x0, y0 + radius,PenColor);
	LCD_PutPixel(x0, y0 - radius,PenColor);
	LCD_PutPixel(x0 + radius, y0,PenColor);
	LCD_PutPixel(x0 - radius, y0,PenColor);
 
	while(x < y) {
		if(f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		LCD_PutPixel(x0 + x, y0 + y,PenColor);
		LCD_PutPixel(x0 - x, y0 + y,PenColor);
		LCD_PutPixel(x0 + x, y0 - y,PenColor);
		LCD_PutPixel(x0 - x, y0 - y,PenColor);
		LCD_PutPixel(x0 + y, y0 + x,PenColor);
		LCD_PutPixel(x0 - y, y0 + x,PenColor);
		LCD_PutPixel(x0 + y, y0 - x,PenColor);
		LCD_PutPixel(x0 - y, y0 - x,PenColor);
	}
}

/**
 * Draw ellipse with centerpont an "radius" linewidth=1 pixel, no fill
 *
 * Parameter:	xy centerpoint and "x,y radius"
 * Return:		void
 */
void LCD_DrawEllipse(int xm, int ym, int a, int b) {
	int dx = 0, dy = b; /* im I. Quadranten von links oben nach rechts unten */
	long a2 = a*a, b2 = b*b;
	long err = b2-(2*b-1)*a2, e2; /* Fehler im 1. Schritt */
 
	do {
		LCD_PutPixel(xm+dx, ym+dy,PenColor); /* I. Quadrant */
		LCD_PutPixel(xm-dx, ym+dy,PenColor); /* II. Quadrant */
		LCD_PutPixel(xm-dx, ym-dy,PenColor); /* III. Quadrant */
		LCD_PutPixel(xm+dx, ym-dy,PenColor); /* IV. Quadrant */
 
		e2 = 2*err;
		if (e2 <  (2*dx+1)*b2) { dx++; err += (2*dx+1)*b2; }
		if (e2 > -(2*dy-1)*a2) { dy--; err -= (2*dy-1)*a2; }
	} 

	while (dy >= 0);
 
	while (dx++ < a) { /* fehlerhafter Abbruch bei flachen Ellipsen (b=1) */
		LCD_PutPixel(xm+dx, ym,PenColor); /* -> Spitze der Ellipse vollenden */
		LCD_PutPixel(xm-dx, ym,PenColor);
	}
}

/**
 * Draw rectangle, with pencolor and fillcolor
 *
 * Parameter:	two diagonal points and  linewidth
 * Return:		void
 */
void LCD_DrawRect(uint8 x0,uint8 y0,uint8 x1,uint8 y1,uint8 line) {
	uint8 x,y;

	y = y0;
	while(y<=y1) {		// zeilenweise durcharbeiten
		x = x0;
		while(x<=x1) {
			if((y < y0 + line) || (y > y1 - line) || (x < x0 + line) || (x > x1 - line)) {
				LCD_PutPixel(x,y,PenColor);
			} else {
				if(      FillColor == 0) LCD_PutPixel(x,y,0);
				else if(FillColor == 1) LCD_PutPixel(x,y,1);
			}

			x++;
		}
		y++;
	}
}

/**
 * Print String
 *
 * Parameter:	x,y startpoint, string
 * Return:		void
 */
void LCD_PrintXY(uint8 x0,uint8 y0,char *s, char inverted, char clearBG) {
	uint8	ix,iy,y;
	const uint8 *font;
	const uint8 *pt;
	uint8 	d;
	char c;
	uint8 char_width,char_height,char_size;
	uint8 i_char_height;
	
	char pixel = 1;
	char noPixel = 0;

	if (inverted) {
		pixel = 0;
		noPixel = 1;
	}

	if(     FontNumber == 1) font = font_fixedsys_8x15;
	else if(FontNumber == 2) font= font_lucida_10x16;
	else if(FontNumber == 3) font = font_terminal_12x16;
	else					 font = font_terminal_6x8;

	char_size =  *font++;
	char_width  = *font++;
	char_height = *font++;
	
	while(*s) {
		c = 0;
		if(*s > 31) c = (*s) - 32;
		pt = &font[(uint16)c * char_size];		
		i_char_height = char_height;
		
		y = y0;
				
		while(i_char_height) {
			for(ix=0;ix<char_width;ix++) {
				d = *pt++;
				for(iy=0;iy<8;iy++)	{	// je ein Byte vertikal ausgeben
					if(d & (1<<iy)) {
						LCD_PutPixel(x0+ix,y+iy, pixel);
					} else {
						if (clearBG) {
							LCD_PutPixel(x0+ix,y+iy, noPixel);
						}
					}
				}
			}

			i_char_height = (i_char_height >= 8) ? i_char_height - 8 : 0; 
			y+=8;		// nächste "Page"
		}	

		x0+=char_width;	
		s++;		// nächstes Zeichen
	}
}

/**
 * Draw bitmap at startingpoint
 *
 * Parameter:	Startpoint xy, pointer to bitmaparray
 * Return:		void
 */
void LCD_DrawBitmap(uint8 x0,uint8 y0,const uint8 *bmp) {
	uint8	width,height;
	uint16	ix,iy;
	
	width     = *bmp++;
	height    = *bmp++;
			
	for(iy=0; iy < height; iy++) {

		for(ix = 0; ix < width; ix++) {

			if(bmp[ix+((iy/8)*width)] & (1<<(iy & 7))) {
				LCD_PutPixel(x0+ix,y0+iy,1);
			} else {
				LCD_PutPixel(x0+ix,y0+iy,0);
			}

		}

	}
}

//--------------------------------------------------------------------------------------------------
// Name:      LCD_DrawBitmapFromFile
// Function:  Draw bitmap from file        UNDER CONSTRUCTION !!!
// bmp format, siehe http://de.wikipedia.org/wiki/Windows_Bitmap
//            
// Parameter: 
// Return:    
//--------------------------------------------------------------------------------------------------
char LCD_DrawBitmapFromFile(uint8 x,uint8 y, const char *filename) {
	char ix, iy, px, i;
	uint16 colorDepth;
	uint32 ofset, width32, width, height, compression;

	FILE *fp;
	char fileData[2200];														// filesize max 2200 bytes;

	fp = fopen(filename, "rb");													// open as file
	if (fp == NULL) {
		printf ("could not open file %s \n", filename);
		return 1;																// error 1 -> could not open file
	}

	if (!fread(fileData, 1, 1100, fp)) {
		printf ("could not read file %s \n", filename);
		return 2;																// error 2 -> could not read file
	}

	if (fileData[0] != 0x42 && fileData[1] != 0x4D) {
		printf ("not a bmp file %s \n", filename);
		return 3;																// error 3 -> not a bmp file ("BM" forst 2 bytes)
	}

	memcpy(&ofset,       &fileData[10], 4);										// ofset -> begin of bmp data
	memcpy(&width,       &fileData[18], 4);										// physical image width
	memcpy(&height,      &fileData[22], 4);										// physical image height
	memcpy(&colorDepth,  &fileData[28], 2);										// color depth of image
	memcpy(&compression, &fileData[30], 4);										// compression infos

	if (colorDepth > 1 || compression != 0) {
		printf ("wrong file format. colordepth: (%d) or compression: (%lu) \n", colorDepth, compression);
		return 4;																// error 4 -> wrong file format (colordepth or compression)
	}

	if (width > 256 || height > 256) {
		printf ("width (%lu) or height (%lu) of image > 256px \n", width, height);
		return 5;																// error 5 -> width or height of image > 256px
	}

	width32 = width;
	if (width32 % 32) {
		width32 = ( (width /32) + 1) * 32;										// width muss ein fielfaches von 32 sein
	}

	for(iy = height; iy > 0; iy--) {
		for( ix = 0; ix < width32; ix=ix+i) {
			for(i = 0; i < 8; i++) {
				px = ( (fileData[ofset] & (0x80 >> i)) || (ix+i) >= width) ? 0 : 1;
				LCD_PutPixel(x + ix + i, y + iy, px);
			}
			ofset++;
		}
	}

	return 0;
}

/**
 * Transfer Framebuffer (RAM) to display via SPI
 *
 * Parameter:	void
 * Return:		void
 */
void LCD_WriteFramebuffer(void) {

	/*
	 * workaround:
	 * at each framebufferwrite we set lcd startline at line 0
	 * This prevent displaced display output after some time.
	 */
	lcd_write_cmd(0x40);

	uint8	 x;
	lcd_set_xy(0,0);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][0]);
	lcd_set_xy(0,1);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][1]);
	lcd_set_xy(0,2);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][2]);
	lcd_set_xy(0,3);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][3]);
	lcd_set_xy(0,4);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][4]);
	lcd_set_xy(0,5);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][5]);
	lcd_set_xy(0,6);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][6]);
	lcd_set_xy(0,7);	for(x=0;x<LCD_WIDTH;x++)  lcd_write_data(framebuffer[x][7]);

	framebufferShouldRefresh = 0;
}

/**
 * Kontrasteinstellung
 *
 * Parameter:	Kontrast (0..63)
 * Return:		void
 */
void LCD_SetContrast(uint8 contrast) {
	lcd_write_cmd(0x81);
	lcd_write_cmd(contrast);
}

/**
 * Init displaycontroller
 *
 * Parameter:	void
 * Return:		void
 */
void LCD_Init(void) {
	// Reset LCD-Display
	rpiHW_lcd_rstClear;
	rpiHW_sleep(50);		// Wait 200ms

	rpiHW_lcd_rstSet;
	rpiHW_sleep(200);		// Wait 200ms

	lcd_write_cmd(0xE2);	// Reset
	lcd_write_cmd(0x40);	// Set display start line (00)
//	lcd_write_cmd(0x5E);	// Set display start line (30)

	if (lcdFlip == 1) {
		lcd_write_cmd(0xA1);	// ADC reset (display mirror horizontal)
		lcd_write_cmd(0xC0);	// Output status select register (display mirror vertical)
		LCD_xOffset = 4;
	} else {
		lcd_write_cmd(0xA0);	// ADC set (display normal horizontal)
		lcd_write_cmd(0xC8);	// Output status select register (display normal vertical)
		LCD_xOffset = 0;
	}

	lcd_write_cmd(0xA4);	// Entire display off (A5: entire display on)
	lcd_write_cmd(0xA6);	// Normal / invert display (A7: invert)
	lcd_write_cmd(0xA2);	// reset LCD bias (A3: set bias)
	lcd_write_cmd(0x2F);	// Set power control ???
	lcd_write_cmd(0x27);	// V0 voltage regulator internal resistor ratio Set ???
	
	lcd_write_cmd(0x81);	// The electronic volume mode set ???
	lcd_write_cmd(8);		// Electronic volume register set ???

//	lcd_write_cmd(0xFA);	// Test mode reset ???
//	lcd_write_cmd(0x90); //???
	lcd_write_cmd(0xAF);	// Turns display on (AE: display off)

	LCD_ClearScreen();
}

/**
 * Kommandobyte an Display senden
 *
 * Parameter:	Byte
 * Return:		void
 */
void lcd_write_cmd(uint8 d) {
	rpiHW_lcd_csClear;
	rpiHW_lcd_rsClear;			// Command Mode
	rpiHW_spiPutc(d);
	rpiHW_lcd_csSet;
}

/**
 * Datenbyte an Display senden
 *
 * Parameter:	Byte
 * Return:		void
 */
void lcd_write_data(uint8 d) {
	rpiHW_lcd_csClear;
	rpiHW_lcd_rsSet;			// Data Mode
	rpiHW_spiPutc(d);
	rpiHW_lcd_csSet;
}

/**
 * Position für Ausgabe festlegen (Koordinaten)
 *
 * Parameter:	X Position (Pixel), Y Position (Page / 8 Pixel Blocks)
 * Return:		void
 */
void lcd_set_xy(uint8 x,uint8 ypage) {
	x += LCD_xOffset;
	lcd_write_cmd(0x00 + (x & 0x0F));
	lcd_write_cmd(0x10 + ((x>>4) & 0x0F));
	lcd_write_cmd(0xB0 + (ypage & 0x07));
}
