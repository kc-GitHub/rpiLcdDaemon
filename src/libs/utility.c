/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    utility.c
 * Description: Some helper methods
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

//=== Includes ================================================================

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>
#include "utility.h"

//=== Golbal variables =========================================================

int debug = 0;

//=============================================================================

/**
 * Split text at given delimiters
 *
 * Parameter:	char	*text			Text to split
 * Parameter:	char	*delims			Delimiters for split
 * Parameter:	int		*elementCount	counted elements after split
 * Return:		char	array
 */
char **textSplit(char *text, char *delims, int *elementCount) {
	char **retVal = NULL;
	char *ptr;
	char *rest; // to point to the rest of the string after token extraction.

	ptr = (char *) strtok_r(text, delims, &rest);
	while(ptr != NULL) {
		retVal = (char **)realloc(retVal, ((*elementCount) + 1) * sizeof(char *));
		retVal[(*elementCount)++] = strdup(ptr);

		// set pointer to next secion
	 	ptr = strtok_r(NULL, delims, &rest	);
	}

	return retVal;
}

/**
 * Remove spaces from begining and end of an string
 * from https://github.com/wirebrush4spam/v.1.1.0/blob/master/debug/string_util.c
 *
 * Parameter:	char	b
 * Return:		char
 */
char* trim(char* b){
	char* e=strrchr(b, '\0'); /* Find the final null */
	while(b<e && isspace(*b)) /* Scan forward */
		++b;

	while (e>b && isspace(*(e-1))) /* scan back from end */
		--e;

	*e='\0'; /* terminate new string */

	return b;
}

/**
 * Write error messages to syslog
 *
 * Parameter:	char	b
 * Parameter:	...
 * Return:		void
 */
void error(char *string, ...) {
	va_list argzeiger;
	char buffer[255];

	va_start(argzeiger,string);
	vsprintf(buffer, string,argzeiger);
	va_end(argzeiger);

	printf("%s \n", buffer);

	syslog (LOG_ERR, "%i: ERROR: %s", getpid(), buffer);
	syslog (LOG_ERR, "%i: Terninated", getpid());

	exit(1);
}

/**
 * Write debug messages to syslog
 *
 * Parameter:	char	b
 * Parameter:	...
 * Return:		void
 */
void syslogDebug(char *string, ...) {
	if (debug) {
		va_list argzeiger;
		char buffer[255];

		va_start(argzeiger,string);
		vsprintf(buffer, string,argzeiger);
		va_end(argzeiger);

		syslog (LOG_DEBUG, "%i: DEBUG: %s", getpid(), buffer);
	}
}

/**
 * Write info messages to syslog
 *
 * Parameter:	char	b
 * Parameter:	...
 * Return:		void
 */
void syslogInfo(char *string, ...) {
	va_list argzeiger;
	char buffer[255];

	va_start(argzeiger,string);
	vsprintf(buffer, string,argzeiger);
	va_end(argzeiger);

	syslog (LOG_INFO, "%i: INFO: %s", getpid(), buffer);
}

/**
 * Write warning messages to syslog
 *
 * Parameter:	char	b
 * Parameter:	...
 * Return:		void
 */
void syslogWarning(char *string, ...) {
	va_list argzeiger;
	char buffer[255];

	va_start(argzeiger,string);
	vsprintf(buffer, string,argzeiger);
	va_end(argzeiger);

	syslog (LOG_WARNING, "%i: WARNING: %s", getpid(), buffer);
}
