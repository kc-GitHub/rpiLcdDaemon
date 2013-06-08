/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    command.h
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

#ifndef COMMAND_H

	#include "std_c.h"

	#define COMMAND_H

#endif /* COMMAND_H */

char *analyseCommand (char buffer[256]);

char *cmd_cls(void);
void cmd_shutdown (void);

char *cmd_text(int x, int y, char *text, char inverted, char clearBG);
char *cmd_line(int x1, int y1, int x2, int y2);
char *cmd_circle(int x, int y, int r);
char *cmd_rect(int x1, int y1, int x2, int y2, int lineWidth);
char *cmd_pixel(int x, int y);
char *cmd_bmp(int x, int y, char *file);

char *cmd_setContrast(int contrast);
char *cmd_setBacklight(uint16 backlight);
char *cmd_setFont(int font);
char *cmd_setFillColor(int color);
char *cmd_setPenColor(int color);

char *cmd_setLed(char led, char status);
