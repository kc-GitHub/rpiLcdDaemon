/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    rpiLcdDaemon.c
 * Description: Small daemon for accessing LCD Display, some LED's and buttons
 *              connected to the Raspberry Pi.
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
	char printUsage = (argc < 2) ? 1 : 0;

	if ( (argv[2] != NULL) && (!printUsage) ) {
		if (strcmp(argv[2], "DEBUG") == 0) {
			debug = 1;
			static char *debugInfo = "Start with DEBUG-INFO";
			syslogDebug(debugInfo);
			printf("%s \n", debugInfo);
		} else {
			printUsage = 1;
		}
	}

	if (printUsage) {
		error ("Usage: %s <portNum> [DEBUG]", argv[0]);
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

