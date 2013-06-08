/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    command.c
 * Description: Controller for dispatching commands received via socket connection
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
#include <errno.h>

#include "utility.h"
#include "timer.h"
#include "command.h"
#include "rpiHardware.h"
#include "lcd.h"

//==============================================================================

/**
 * Analyse incomming commands
 *
 * Parameter:	char	*buffer		incomming command buffer
 * Return:		char
 */
char *analyseCommand (char buffer[256]) {
	char *response = "???                         ";

	int commandCount = 0;
	char **commandArray = textSplit(buffer, ",", &commandCount);
	char *command = commandArray[0];

	stopTimer();

	if (strcmp(command, "cls") == 0) {
		response = cmd_cls();

	} else if (strcmp(command, "setContrast") == 0) {
		if (commandCount == 2) {
			int contrast = atoi(commandArray[1]);
			if (contrast >= 0 && contrast < 64) {
				response = cmd_setContrast(contrast);
			} else {
				response = "Contrast must be (0 ... 63)";
			}
		} else {
			response = "usage: setContrast,0...63";
		}

	} else if (strcmp(command, "setBacklight") == 0) {
		if (commandCount == 2) {
			int backlight = atoi(commandArray[1]);
			response = cmd_setBacklight(backlight);
		} else {
			response = "usage: setBacklight,0...254";
		}

	} else if (strcmp(command, "setFont") == 0) {
		if (commandCount == 2) {
			int fontNum = atoi(commandArray[1]);
			if (fontNum >= 0 && fontNum < 4) {
				response = cmd_setFont(fontNum);
			} else {
				response = "unknown font number (0 ... 3)";
			}
		} else {
			response = "usage: setFont,fontNum";
		}

	} else if (strcmp(command, "time") == 0) {
		if (commandCount == 5) {
			displayTime.start = 1;
			displayTime.x = atoi(commandArray[1]);
			displayTime.y = atoi(commandArray[2]);
			displayTime.font = atoi(commandArray[3]);
			displayTime.additional = atoi(commandArray[4]);
			oldSecond = 255;
			response = "show time";
		} else {
			response = "usage: time,x,y,fontNum,showSeconds";
		}

	} else if (strcmp(command, "stopTime") == 0) {
		if (commandCount == 1) {
			displayTime.start = 0;
			response = "stop time";
		} else {
			response = "usage: stopTime";
		}

	} else if (strcmp(command, "date") == 0) {
		if (commandCount == 5) {
			displayDate.start = 1;
			displayDate.x = atoi(commandArray[1]);
			displayDate.y = atoi(commandArray[2]);
			displayDate.font = atoi(commandArray[3]);
			displayDate.additional = atoi(commandArray[4]);
			oldMinute = 255;
			response = "show date";
		} else {
			response = "usage: date,x,y,fontNum,showCentury";
		}

	} else if (strcmp(command, "stopDate") == 0) {
		if (commandCount == 1) {
			displayDate.start = 0;
			response = "stop date";
		} else {
			response = "usage: stopDate";
		}

	} else if (strcmp(command, "setFillColor") == 0) {
		if (commandCount == 2) {
			response = cmd_setFillColor(atoi(commandArray[1]));
		} else {
			response = "usage: setFillColor,0|1";
		}

	} else if (strcmp(command, "setPenColor") == 0) {
		if (commandCount == 2) {
			response = cmd_setPenColor(atoi(commandArray[1]));
		} else {
			response = "usage: setPenColor,1|0";
		}


	} else if (strcmp(command, "text") == 0) {
		if (commandCount == 6) {
			int x = atoi(commandArray[1]);
			int y = atoi(commandArray[2]);
			char *text = (char*)commandArray[3];
			char inverted = atoi(commandArray[4]);
			char clearBG = atoi(commandArray[5]);
			response = cmd_text(x, y, text, inverted, clearBG);
		} else {
			response = "usage: text,x,y,text_to_write,inverted,clearBG";
		}

	} else if (strcmp(command, "line") == 0) {
		if (commandCount == 5) {
			int x1 = atoi(commandArray[1]);
			int y1 = atoi(commandArray[2]);
			int x2 = atoi(commandArray[3]);
			int y2 = atoi(commandArray[4]);
			response = cmd_line(x1, y1, x2, y2);
		} else {
			response = "usage: line,x1,y1,x2,y2";
		}

	} else if (strcmp(command, "circle") == 0) {
		if (commandCount == 4) {
			int x = atoi(commandArray[1]);
			int y = atoi(commandArray[2]);
			int r = atoi(commandArray[3]);
			response = cmd_circle(x, y, r);
		} else {
			response = "usage: circle,x,y,r";
		}

	} else if (strcmp(command, "rect") == 0) {
		if (commandCount == 6) {
			int x1 = atoi(commandArray[1]);
			int y1 = atoi(commandArray[2]);
			int x2 = atoi(commandArray[3]);
			int y2 = atoi(commandArray[4]);
			int lineWidth = atoi(commandArray[5]);
			response = cmd_rect(x1, y1, x2, y2, lineWidth);
		} else {
			response = "usage: rect,x1,y1,x2,y2,w";
		}

	} else if (strcmp(command, "pixel") == 0) {
		if (commandCount == 3) {
			int x = atoi(commandArray[1]);
			int y = atoi(commandArray[2]);
			response = cmd_pixel(x, y);
		} else {
			response = "usage: pixel,x,y";
		}

	} else if (strcmp(command, "bmp") == 0) {
		if (commandCount == 4) {
			int x = atoi(commandArray[1]);
			int y = atoi(commandArray[2]);
			char *file = commandArray[3];
			response = cmd_bmp(x, y, file);
		} else {
			response = "usage: bmp,x,y,file";
		}

	} else if (strcmp(command, "setLed") == 0) {
		if (commandCount == 3) {
			int led = atoi(commandArray[1]);
			int status = atoi(commandArray[2]);
			response = cmd_setLed(led, status);
		} else {
			response = "usage: setLed,ledNum,status";
		}

	} else {
		response = "Unknown Command.";
	}

	startTimer();

	return response;
}

/* ### all command functions ################################################ */

/**
 * Dispatcher for command setFont
 *
 * Parameter:	int		font	font number
 * Return:		char
 */
char *cmd_setFont(int font) {
	char buffer[80] = "";

	LCD_SetFont(font);

	sprintf(buffer, "Font %i is set.", font);
	return strdup(buffer);
}

/**
 * Dispatcher for command setContrast
 *
 * Parameter:	int		contrast	contrast value
 * Return:		char
 */
char *cmd_setContrast(int contrast) {
	char buffer[80] = "";

	LCD_SetContrast(contrast);

	sprintf(buffer, "Contrast %i is set.", contrast);
	return strdup(buffer);
}

/**
 * Dispatcher for command setBacklight
 *
 * Parameter:	uint16		backlight	backlight value
 * Return:		char
 */
char *cmd_setBacklight(uint16 backlight) {
	char buffer[80] = "Backlight must be (0 ... 255)";

	if (backlight >= 0 && backlight <= 255) {
		sprintf(buffer, "Backlight %i is set.", backlight);
		setBacklight(backlight);
	}

	return strdup(buffer);
}

/**
 * Dispatcher for command setFillColor
 *
 * Parameter:	int		color	fill color
 * Return:		char
 */
char *cmd_setFillColor(int color) {
	char buffer[80] = "";

	LCD_SetFillColor(color);

	sprintf(buffer, "FillColor %i is set.", color);
	return strdup(buffer);
}

/**
 * Dispatcher for command setPenColor
 *
 * Parameter:	int		color	pen color
 * Return:		char
 */
char *cmd_setPenColor(int color) {
	char buffer[80] = "";

	LCD_SetPenColor(color);

	sprintf(buffer, "PenColor %i is set.", color);
	return strdup(buffer);
}

/**
 * Dispatcher for command text
 *
 * Parameter:	int		x
 * Parameter:	int		y
 * Parameter:	int		text
 * Parameter:	char	inverted	set to 1 to invert the text output
 * Parameter:	char	clearBG		set to 1 if the background should cleared
 * Return:		char
 */
char *cmd_text(int x, int y, char *text, char inverted, char clearBG) {
	char buffer[80] = "";

	LCD_PrintXY(x, y, text, inverted, clearBG);

	sprintf(buffer, "Show \"%s\" at pos %i,%i.", text, x, y);
	return strdup(buffer);
}

/**
 * Dispatcher for command line
 *
 * Parameter:	int		x1
 * Parameter:	int		y1
 * Parameter:	int		x2
 * Parameter:	int		y2
 * Return:		char
 */
char *cmd_line(int x1, int y1, int x2, int y2) {
	char buffer[80] = "";

	LCD_DrawLine(x1, y1, x2, y2);

	sprintf(buffer, "Draw line from %i, %i to %i, %i", x1, y1, x2, y2);
	return strdup(buffer);
}

/**
 * Dispatcher for command circle
 *
 * Parameter:	int		x
 * Parameter:	int		y
 * Parameter:	int		r
 * Return:		char
 */
char *cmd_circle(int x, int y, int r) {
	char buffer[80] = "";

	LCD_DrawCircle(x, y, r);

	sprintf(buffer, "Draw circle at %i, %i R %i", x, y, r);
	return strdup(buffer);
}

/**
 * Dispatcher for command setBacklight
 *
 * Parameter:	int		x1
 * Parameter:	int		y1
 * Parameter:	int		x2
 * Parameter:	int		y2
 * Parameter:	int		lineWidth
 * Return:		char
 */
char *cmd_rect(int x1, int y1, int x2, int y2, int lineWidth) {
	char buffer[80] = "";

	LCD_DrawRect(x1, y1, x2, y2, lineWidth);

	sprintf(buffer, "Draw rect from %i, %i to %i, %i lineWidth %i", x1, y1, x2, y2, lineWidth);
	return strdup(buffer);
}

/**
 * Dispatcher for command pixel
 *
 * Parameter:	int		x
 * Parameter:	int		y
 * Return:		char
 */
char *cmd_pixel(int x, int y) {
	char buffer[80] = "";

	LCD_PutPixel(x, y, LCD_GetPenColor());

	sprintf(buffer, "Draw pixel at %i, %i", x, y);
	return strdup(buffer);
}

/**
 * Dispatcher for command bmp
 *
 * Parameter:	int		x
 * Parameter:	int		y
 * Parameter:	char	*filename	filename of bmp file to display
 * Return:		char
 */
char *cmd_bmp(int x, int y, char *filename) {
	char buffer[160] = "";

	char result = LCD_DrawBitmapFromFile(x, y, filename);

	if (result) {
		sprintf(buffer, "Error on draw bitmap: Error_%i, filename: %s", result, filename);
	} else {
		sprintf(buffer, "Draw bmp file at %i, %i filename: %s", x, y, filename);
	}
	return strdup(buffer);
}

/**
 * Dispatcher for command setLed
 *
 * Parameter:	char	led		Led number to controll
 * Parameter:	char	statue	Set 1 to switch led on, 0 to switch led off
 * Return:		char
 */
char *cmd_setLed(char led, char status) {
	char buffer[160] = "";

	setLed(led, status);

	sprintf(buffer, "Set LED: %i to %i", led, status);
	return strdup(buffer);
}

/**
 * Dispatcher for command cls
 *
 * Parameter:	void
 * Return:		char
 */
char *cmd_cls(void) {
	LCD_ClearScreen();
	oldSecond = 255;
	oldMinute = 255;

	return "Screen cleared.";
}

/**
 * Dispatcher for command shutdown
 * This exit the daemon
 *
 * Parameter:	void
 * Return:		void
 */
void cmd_shutdown (void) {
	syslogInfo("Daemon exiting now!");
	exit(EXIT_SUCCESS);
}
