/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    rpiLcdDaemon.h
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

#ifndef RPI_LCD_DAEMON_H

	void init(void);
	void showLoadingScreen(void);
	void signalHandler(int sig);

	#define RPI_LCD_DAEMON_H

#endif /* RPI_LCD_DAEMON_H */

