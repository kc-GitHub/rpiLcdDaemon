/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    rpiHardware.c
 * Description: Hardware related methodes for Raspberry Pi
 * Author:      Dirk Hoffmann <hoffmann@vmd-jena.de> 2013
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
 *
 * Dieses Programm ist Freie Software: Sie k�nnen es unter den Bedingungen der
 * GNU General Public License, wie von der Free Software Foundation, Version 3
 * der Lizenz oder (nach Ihrer Option) jeder späteren veröffentlichten Version,
 * weiterverbreiten und/oder modifizieren.
 *
 * Dieses Programm wird veröffentlicht in der Hoffnung, dass es nützlich sein
 * wird, aber OHNE JEDE GEWÄHRLEISTUNG; sogar ohne die implizite Gewährleistung
 * der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK. Siehe die GNU
 * General Public License f�r weitere Details.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
 *******************************************************************************
 *
 * History:     25.05.2013 Initial version V0.0.1
 *
 *******************************************************************************/

//=== Includes =================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include <pthread.h>

#include <signal.h>

#include <syslog.h>


#include <wiringPi.h>
//#include <wiringPiSPI.h>

#include "std_c.h"
#include "utility.h"
#include "timer.h"
#include "server.h"
#include "lcd.h"
#include "rpiHardware.h"
#include "command.h"

//=== Global variables =========================================================

int buttonStatus = 0;
int buttonStatusOld = 0;
char buttonTimer = 0;

volatile sig_atomic_t buttonCounter = 0;
volatile sig_atomic_t buttonCounterOld = 0;

//==============================================================================

/**
 * Initializing all required hardware of the Raspberry Pi
 *
 * Parameter:	void
 * Return:		void
 */
void initHardware(void) {
	int isrResult;

	if (wiringPiSetup () < 0) {
		error("Unable to setup wiringPi: %s", strerror (errno));
	}

	// Init backlight LED-Port
	pullUpDnControl(LED_BACKLIGH, PUD_OFF);	// Disable Pullup
	if (LED_BACKLIGH == 1) {
		pinMode (LED_BACKLIGH, PWM_OUTPUT);
		pinMode (LED_BACKLIGH, PWM_OUTPUT);
		pwmSetMode (PWM_MODE_MS);
		pwmSetMode (PWM_MODE_MS);
		pwmSetRange (255);
		pwmSetRange (255);

		pwmSetClock (10);
		pwmSetClock (10);
	} else {
		pinMode (LED_BACKLIGH, OUTPUT);
	}

	// Init LED-Ports
	pullUpDnControl(LED_GREEN,  PUD_OFF);	// Disable Pullup
	pullUpDnControl(LED_YELLOW, PUD_OFF);	// Disable Pullup
	pullUpDnControl(LED_RED,    PUD_OFF);	// Disable Pullup

	pinMode (LED_GREEN,  OUTPUT);			// LED1 (green);
	pinMode (LED_YELLOW, OUTPUT);			// LED2 (yellow);
	pinMode (LED_RED,    OUTPUT);			// LED3 (red);

	// Buttons
	pinMode(BTN_LEFT,   INPUT);				// Btn 3, GPIO2 (Pin 13)
	pinMode(BTN_CENTER, INPUT);				// Btn 2, GPIO3 (Pin 15)
	pinMode(BTN_RIGHT,  INPUT);				// Btn 1, GPIO4 (Pin 16)

	pullUpDnControl(BTN_LEFT,   PUD_UP);	// Enable Pullup
	pullUpDnControl(BTN_CENTER, PUD_UP);	// Enable Pullup
	pullUpDnControl(BTN_RIGHT,  PUD_UP);	// Enable Pullup

	// Init PINS for Software SPI to LCD
	pullUpDnControl(PIN_LCD_MOSI, PUD_OFF);	// Disable Pullup
	pullUpDnControl(PIN_LCD_SCLK, PUD_OFF);	// Disable Pullup
	pullUpDnControl(PIN_LCD_RST,  PUD_OFF);	// Disable Pullup
	pullUpDnControl(PIN_LCD_CS,   PUD_OFF);	// Disable Pullup
	pullUpDnControl(PIN_LCD_RS,   PUD_OFF);	// Disable Pullup

	pinMode(PIN_LCD_MOSI, OUTPUT);			// GPIO10 Output: MOSI
	pinMode(PIN_LCD_SCLK, OUTPUT);			// GPIO11 Output: SCLK
	pinMode(PIN_LCD_RST,  OUTPUT);			// GPIO25 Output: RST
	pinMode(PIN_LCD_CS ,  OUTPUT);			// GPIO8  Output: CS
	pinMode(PIN_LCD_RS,   OUTPUT);			// GPIO7  Output: RS

	// Init button ISR's
	isrResult = wiringPiISR (BTN_LEFT,   INT_EDGE_BOTH, &ISR_button);
	delay(100);
	// We initializing the isr again. This is a workaround so sometime the
	// button don't fire a interupt
	isrResult = wiringPiISR (BTN_LEFT,   INT_EDGE_BOTH, &ISR_button);
	delay(100);

	isrResult = wiringPiISR (BTN_CENTER, INT_EDGE_BOTH, &ISR_button);
	delay(100);
	// We initializing the isr again. This is a workaround so sometime the
	// button don't fire a interupt
	isrResult = wiringPiISR (BTN_CENTER, INT_EDGE_BOTH, &ISR_button);
	delay(100);

	isrResult = wiringPiISR (BTN_RIGHT,  INT_EDGE_BOTH, &ISR_button);
	delay(100);
	// We initializing the isr again. This is a workaround so sometime the
	// button don't fire a interupt
	isrResult = wiringPiISR (BTN_RIGHT,  INT_EDGE_BOTH, &ISR_button);

	if (isrResult < 0) {
		syslogWarning ("Unable to setup wiringPi ISR: %s", strerror (errno));
	}
}

/**
 * Handles button flags each timer intervall
 *
 * Parameter:	void
 * Return:		void
 */
void btn_Handler(void) {
	const char maxButtonTimer = 10;

	if ( (buttonStatus & 0b1) || (buttonStatus & 0b100) || (buttonStatus & 0b10000) ) {
		if (buttonTimer < maxButtonTimer) {		// long keypress > 3 sec.
			buttonTimer++;
			if (buttonTimer >= maxButtonTimer) {
				buttonCounter++;
			}
		}
	} else {
		if (buttonTimer > 0) {
			buttonCounter++;
		}
		buttonTimer = 0;
	}

	if (buttonCounter != buttonCounterOld) {
		int btnLeft   = digitalRead(BTN_LEFT);
		int btnCenter = digitalRead(BTN_CENTER);
		int btnRight  = digitalRead(BTN_RIGHT);

		if (buttonStatus != buttonStatusOld) {
			buttonTimer = 0;
		}

		if (!btnLeft) {
			buttonStatus |=  (1 << 0);
			if (buttonTimer >= maxButtonTimer) {
				buttonStatus |=  (1 << 1);
			}
		} else {
			buttonStatus &= ~(1 << 0);
			buttonStatus &= ~(1 << 1);
		}

		if (!btnCenter) {
			buttonStatus |=  (1 << 2);
			if (buttonTimer >= maxButtonTimer) {
				buttonStatus |=  (1 << 3);
			}
		} else {
			buttonStatus &= ~(1 << 2);
			buttonStatus &= ~(1 << 3);
		}

		if (!btnRight) {
			buttonStatus |=  (1 << 4);
			if (buttonTimer >= maxButtonTimer) {
				buttonStatus |=  (1 << 5);
			}
		} else {
			buttonStatus &= ~(1 << 4);
			buttonStatus &= ~(1 << 5);
		}

		// press all 3 buttons to shutdown the system
		if (!btnLeft && !btnCenter && !btnRight) {
			cmd_cls();
			cmd_setFont(1);
			cmd_text(23, 25, "Shutdown now", 0, 1);
			syslogInfo ("Shutdown keys pressed. Initiating shutdown");
			system("/sbin/halt");
		}

		buttonCounterOld = buttonCounter;
	}

	if (firstClient && buttonStatus != buttonStatusOld) {
		buttonStatusOld = buttonStatus;

		char buffer[36];

		sprintf(buffer, "ButtonStatus: %i, buttonCounter: %i\n", buttonStatus, buttonCounter);
		write(firstClient , buffer , strlen(buffer));
	}
}

/**
 * Interupt service routine (ISR) for button relates interrupts
 *
 * Parameter:	void
 * Return:		void
 */
void ISR_button	(void) {
	buttonCounter++;
	syslogDebug ("ISR_button: buttonCounter: %i", buttonCounter);
}

/**
 * Set backlight to given value
 *
 * Parameter:	void
 * Return:		void
 */
void setBacklight (uint8 value) {
	if (LED_BACKLIGH == 1) {
		pwmWrite(LED_BACKLIGH, value);
	} else {
		digitalWrite (LED_BACKLIGH, (value) ? 1 : 0);
	}
}

/**
 * Set specific LED to given value (on/off)
 *
 * Parameter:	char	led		Number of led to controll
 * Parameter:	char	status	Set to 1 = set led on, 0 = led off
 * Return:		void
 */
void setLed(char led, char status) {
	char gpio = LED_GREEN;

	if(led == 2) {
		gpio = LED_YELLOW;
	} else if(led == 3) {
		gpio = LED_RED;
	}

	digitalWrite (gpio, (status) ? 1 : 0);

}

/**
 * Let sleep the program for some ms
 *
 * Parameter:	uint32	ms		Number milliseconds to sleep
 * Return:		void
 */
void rpiHW_sleep(uint32 ms) {
	delay(ms);
}

/**
 * wait some cpu cycles
 *
 * Parameter:	void
 * Return:		void
 */
void rpiHW_spiWait(void) {
	int i;
	for(i = 0; i < 25; i++) {
		asm("NOP;");
	}
}

/**
 * Put a byte to the SPI lines
 * Here we use soft SPI, so we need noe special kernel driver
 *
 * Parameter:	unsigned char	byteToSend		Byte to send via SPI
 * Return:		void
 */
void rpiHW_spiPutc(unsigned char byteToSend) {
	int i; //,n;

	ripHW_lcd_sclkClear;
	for(i = 0; i < 8; i++) {

		ripHW_lcd_sclkClear;
		rpiHW_spiWait();

		if(byteToSend & 0x80) {
			ripHW_lcd_mosiSet;
		} else {
			ripHW_lcd_mosiClear;
		}

		byteToSend <<= 1;
		rpiHW_spiWait();

		ripHW_lcd_sclkSet;
		rpiHW_spiWait();
	}
}
