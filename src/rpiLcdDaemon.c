/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    rpiLcdDaemon.c
 * Description: Small daemon for accessing LCD Display, some LED's and buttons
 *              connected to the Raspberry Pi.
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
#include <ctype.h>

#include <pthread.h>

#include <sys/wait.h>
#include <signal.h>

#include "libs/server.h"
#include "libs/command.h"
#include "libs/lcd.h"
#include "libs/utility.h"
#include "libs/timer.h"
#include "libs/rpiHardware.h"

#include "rpiLcdDaemon.h"

#define VERSION 0.4

//==============================================================================

/**
 * This is the main function
 *
 * Parameter:	int		argc
 * Parameter:	char	*argc
 *
 * Return:		int
 */
int main(int argc, char *argv[]) {
	char printUsage = (argc < 2) || !isnumeric(argv[1]) ? 1 : 0;
	debug = 0;
	lcdFlip = 0;
	allowRemote = 0;

	printf("rpiLcdDaemon Version %.1f\n", VERSION);

	uint8 validParamCount = 0;
	if ( (argv[2] != NULL) && !printUsage ) {
		uint8 i;
		for(i = 1; i < argc; i++) {
			if (strcmp(argv[i], "DEBUG") == 0) {
				debug = 1;
				validParamCount++;

			} else if (strcmp(argv[i], "lcdFlip") == 0) {
				lcdFlip = 1;
				validParamCount++;

			} else if (strcmp(argv[i], "allowRemoteConnect") == 0) {
				allowRemote = 1;
				validParamCount++;
			}
		}

		if (validParamCount != (argc -2)) {
			printUsage = 1;
		}
	}

	if (printUsage) {
		error ("Usage: %s <portNum> [allowRemoteConnect] [lcdFlip] [DEBUG]", argv[0]);
	}

	if (debug == 1) {
		static char *debugInfo = "Start with DEBUG-INFO";
		syslogDebug(debugInfo);
		printf("%s \n", debugInfo);

		if (lcdFlip == 1) {
			static char *debugInfo = "LCD content is flipped";
			syslogDebug(debugInfo);
			printf("%s \n", debugInfo);
		}

		if (allowRemote == 1) {
			static char *debugInfo = "Remote access is enabled";
			syslogDebug(debugInfo);
			printf("%s \n", debugInfo);
		}
	}

	int portNum = atoi(argv[1]);

	init();

	framebufferShouldRefresh = 1;

	/* Establish a handler for SIGALRM signals. */
	signal(SIGCHLD, signalHandler);
	signal(SIGINT, signalHandler);

	// Setup and start timer
	signal(SIGALRM, timerHandler);

	startTimer();

	printf ("LCD Daemon start now. PID: %i \nAll messages send to syslog! \n", getpid());
	syslogInfo("LCD Daemon start now");

	// show predefined loading screen
	showLoadingScreen();

	firstClient = 0;
	doShutdown = 0;
	startServer(portNum);

	return 0;
}

/**
 * Call all initializions
 *
 * Parameter:	void
 * Return:		void
 */
void init(void) {
	initHardware();

	LCD_Init();			// Init Display
}

/**
 * Shows a sample "booting" screen an start of the daemon
 *
 * Parameter:	void
 * Return:		void
 */
void showLoadingScreen(void) {
	cmd_setLed(1,1);

	LCD_SetPenColor(1);
	cmd_cls();
	cmd_setContrast(6);

	char *response = cmd_bmp(0, 0, "/opt/rpiLcdDaemon/images/fhem.bmp");
	syslogInfo("%s", response);

	cmd_text(76, 2,  "Home",       0, 1);
	cmd_text(60, 12, "Automation", 0, 1);
	cmd_text(70, 22, "Server",     0, 1);

	cmd_setFont(1);
	cmd_text(50, 34, "booting...",    0, 1);

	cmd_setBacklight(100);
}

/**
 * Funkction for signal handling
 *
 * Parameter:	int		sig
 * Return:		void
 */
void signalHandler(int sig) {
	syslogDebug ("signalHandler %d", sig);

	if (sig == SIGCHLD) {
		wait3(NULL, WNOHANG, NULL);

	} else if (sig == SIGINT) {		// Signal for Strg+C
		cmd_shutdown();
	}
}

