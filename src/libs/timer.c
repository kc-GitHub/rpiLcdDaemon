/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    timer.c
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

//=== Includes ================================================================

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
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

#include "utility.h"
#include "timer.h"
#include "lcd.h"
#include "command.h"
#include "rpiHardware.h"

//==============================================================================

/**
 * Start the timer
 *
 * Parameter:	void
 * Return:		void
 */
void startTimer(void) {
	struct itimerval period = {
		{ 0, 100000, }, /* 1st signal in [s], [us] */
		{ 0, 100000, }, /* period time   [s], [us] */
	};

	setitimer(ITIMER_REAL, &period, NULL); /* start periodic SIGALRM signals */
}

/**
 * Stop the timer
 *
 * Parameter:	void
 * Return:		void
 */
void stopTimer(void) {
	struct itimerval period = {
		{ 0, 0, }, /* 1st signal in [s], [us] */
		{ 0, 0, }, /* period time   [s], [us] */
	};

	setitimer(ITIMER_REAL, &period, NULL); /* start periodic SIGALRM signals */
}

/**
 * Handle each timer interrupt
 * Here we count the date and time for displaying on the lcd
 *
 * Parameter:	int		sig
 * Return:		void
 */
void timerHandler(int sig) {
	int century;
	int oldFont = 0;

	stopTimer();

	// handle button states
	btn_Handler();

	if (displayTime.start || displayDate.start) {
		time_t tnow;
		time(&tnow);
		tmnow = localtime(&tnow);

		oldFont = LCD_GetFont();

		if (displayTime.start && (oldSecond != tmnow->tm_sec) ) {
			LCD_SetFont(displayTime.font);

			if (displayTime.additional) {
				sprintf(displayTime.buffer, "%02d:%02d:%02d", tmnow->tm_hour, tmnow->tm_min, tmnow->tm_sec);
			} else {
				sprintf(displayTime.buffer, "%02d:%02d", tmnow->tm_hour, tmnow->tm_min);
			}

			if (strcmp(displayTime.buffer, displayTime.bufferOld) != 0 || oldSecond == 255) {
				LCD_PrintXY(displayTime.x, displayTime.y, displayTime.bufferOld, 1, 0);
				LCD_PrintXY(displayTime.x, displayTime.y, displayTime.buffer, 0, 0);
				strcpy(displayTime.bufferOld, displayTime.buffer);
			}

			oldSecond = tmnow->tm_sec;
		}

		if (displayDate.start  && (oldMinute != tmnow->tm_min) ) {
			LCD_SetFont(displayDate.font);

			century = tmnow->tm_year;
			if (displayDate.additional) {
				century = century + 1900;
			} else {
				if (tmnow->tm_year > 100) {
					century = century - 100;
				}
			}

			sprintf(displayDate.buffer, "%02d.%02d.%02d", tmnow->tm_mday, tmnow->tm_mon + 1, century);

			if (strcmp(displayDate.buffer, displayDate.bufferOld) != 0 || oldMinute == 255) {
				LCD_PrintXY(displayDate.x, displayDate.y, displayDate.bufferOld, 1, 0);
				LCD_PrintXY(displayDate.x, displayDate.y, displayDate.buffer, 0, 0);
				strcpy(displayDate.bufferOld, displayDate.buffer);
			}

			oldMinute = tmnow->tm_min;
		}

		LCD_SetFont(oldFont);
	}

	if (framebufferShouldRefresh) {
		LCD_WriteFramebuffer();
	}

	startTimer();
}
