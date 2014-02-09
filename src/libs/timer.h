/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    timer.h
 * Description: All timer related methods
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

struct displayDateTime {
	char start;
	char x;
	char y;
	char font;
	char additional;
	char buffer[10];
	char bufferOld[10];
} displayTime, displayDate;

struct tm *tmnow;

char oldSecond;
char oldMinute;

void startTimer(void);
void stopTimer(void);
void timerHandler(int sig);

