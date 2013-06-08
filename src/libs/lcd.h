//--------------------------------------------------------------------------------------------------
//                                  _            _     
//                                 | |          | |    
//      ___ _ __ ___  ___ _   _ ___| |_ ___  ___| |__  
//     / _ \ '_ ` _ \/ __| | | / __| __/ _ \/ __| '_ \. 
//    |  __/ | | | | \__ \ |_| \__ \ ||  __/ (__| | | |
//     \___|_| |_| |_|___/\__, |___/\__\___|\___|_| |_|
//                         __/ |                       
//                        |___/    Engineering (www.emsystech.de)
//
// Filename:    lcd.h
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
// Dieses Programm ist Freie Software: Sie k�nnen es unter den Bedingungen der GNU General Public
// License, wie von der Free Software Foundation, Version 3 der Lizenz oder (nach Ihrer Option) 
// jeder sp�teren ver�ffentlichten Version, weiterverbreiten und/oder modifizieren.
//
// Dieses Programm wird in der Hoffnung, dass es n�tzlich sein wird, aber OHNE JEDE GEW�HRLEISTUNG,
// bereitgestellt; sogar ohne die implizite Gew�hrleistung der MARKTF�HIGKEIT oder EIGNUNG F�R 
// EINEN BESTIMMTEN ZWECK. Siehe die GNU General Public License f�r weitere Details.
//
// Sie sollten eine Kopie der GNU General Public License zusammen mit diesem Programm erhalten 
// haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
//                       
// Author:      Martin Steppuhn
// History:     05.11.2012 Initial version V0.9.0
//
//              Dirk Hoffmann <hoffmann@vmd-jena.de>
//              25.05.2013 Some changes for use with the rpiLcdDaemon
//--------------------------------------------------------------------------------------------------

#ifndef LCD_H
	//=== Includes =====================================================================================
	#include "std_c.h"
	#include <wiringPi.h>

	//=== Preprocessing directives (#define) ===========================================================
	#define		LCD_WIDTH		128
	#define		LCD_HEIGHT		64

	//#define		LCD_X_OFFSET	4		// Pixel Offset
	#define		LCD_X_OFFSET	0		// Pixel Offset

	//=== Type definitions (typedef) ===================================================================

	//=== Global constants (extern) ====================================================================

	//=== Global variables (extern) ====================================================================
	uint8	framebufferShouldRefresh;

	//=== Global function prototypes ===================================================================

	//=== Local function prototypes ====================================================================
	void lcd_write_cmd(uint8 d);
	void lcd_write_data(uint8 d);
	void lcd_set_xy(uint8 x,uint8 ypage);

	void LCD_ClearScreen(void);
	void LCD_SetPenColor(uint8 c);
	void LCD_SetFillColor(int8 c);
	void LCD_SetFont(uint8 f);
	int LCD_GetFont(void);
	void LCD_SetContrast(uint8 contrast);
	uint8 LCD_GetPenColor();

	void LCD_PutPixel(uint8 x,uint8 y,uint8 color);
	void LCD_DrawLine(uint8 x0,uint8 y0,uint8 x1,uint8 y1);
	void LCD_DrawCircle(uint8 x0,uint8 y0,uint8 radius);
	void LCD_DrawEllipse(int xm, int ym, int a, int b);
	void LCD_DrawRect(uint8 x0,uint8 y0,uint8 x1,uint8 y1,uint8 line);
	void LCD_PrintXY(uint8 x0,uint8 y0,char *s, char inverted, char clearBG);
	void LCD_DrawBitmap(uint8 x0,uint8 y0,const uint8 *bmp);
	char LCD_DrawBitmapFromFile(uint8 x0,uint8 y0, const char *filename);

	void LCD_Init(void);
	void LCD_WriteFramebuffer(void);

	#define LCD_H

#endif
