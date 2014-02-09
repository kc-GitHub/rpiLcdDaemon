/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    rpiHardware.h
 * Description: Hardware related methodes for Raspberry Pi
 * Author:      Dirk Hoffmann <hoffmann@vmd-jena.de> 2013-2014
 *
 *******************************************************************************
 * Open Source Licensing
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef RPIHARDWARE_H

	#include <signal.h>
	#include "std_c.h"

	/*
	 * The GPIO-Pins numbering found at: https://projects.drogon.net/raspberry-pi/wiringpi/pins/
	 * We used the wiringPi schema
	 */

	// Defined specific GPIO-Pins
	#define LED_BACKLIGH       1	// Pin-Header 12

	#define BTN_LEFT           2	// Pin-Header 13
	#define BTN_CENTER         3	// Pin-Header 15
	#define BTN_RIGHT          4	// Pin-Header 16

	#define LED_GREEN          8	// Pin-Header 3
	#define LED_YELLOW         9	// Pin-Header 5
	#define LED_RED            7	// Pin-Header 7

	// LCD Pins
	#define PIN_LCD_RST        6	// 25
	#define PIN_LCD_CS        10	// 8
	#define PIN_LCD_RS        11	// 7
	#define PIN_LCD_MOSI      12	// 10
	#define PIN_LCD_SCLK      14	// 11

	// define macros
	#define	rpiHW_lcd_rstClear  digitalWrite(PIN_LCD_RST, 0);
	#define	rpiHW_lcd_rstSet    digitalWrite(PIN_LCD_RST, 1);

	#define	rpiHW_lcd_csClear   digitalWrite(PIN_LCD_CS, 0);
	#define	rpiHW_lcd_csSet     digitalWrite(PIN_LCD_CS, 1);

	#define	rpiHW_lcd_rsClear   digitalWrite(PIN_LCD_RS, 0);
	#define	rpiHW_lcd_rsSet     digitalWrite(PIN_LCD_RS, 1);

	#define	rpiHW_lcd_mosiClear digitalWrite(PIN_LCD_MOSI, 0);
	#define	rpiHW_lcd_mosiSet   digitalWrite(PIN_LCD_MOSI, 1);

	#define	rpiHW_lcd_sclkClear digitalWrite(PIN_LCD_SCLK, 0);
	#define	rpiHW_lcd_sclkSet   digitalWrite(PIN_LCD_SCLK, 1);

	#define RPIHARDWARE_H

#endif /* RPIHARDWARE_H */


extern int buttonStatus;
extern int buttonStatusOld;
extern char buttonTimer;

extern volatile sig_atomic_t buttonCounter;
extern volatile sig_atomic_t buttonCounterOld;

void initHardware (void);
void ISR_button	(void);
void btn_Handler(void);

void setBacklight (uint8 value);
void setLed(char led, char status);

void rpiHW_sleep(uint32 ms);
void rpiHW_spiPutc(unsigned char byteToSend);
