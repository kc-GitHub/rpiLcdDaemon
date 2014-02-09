/*******************************************************************************
 * This file is part of rpiLcdDaemon
 *
 * Filename:    std_c.h
 * Description: Here we declared some data types we need
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

#ifndef STD_C_H
	#define STD_C_H

	// Boolean values
	#define TRUE  1
	#define FALSE 0

	#define true  1
	#define false 0

	//=== Type definitions =====================================================

	// Standard types
	typedef unsigned char  uint8;
	typedef unsigned short uint16;
	typedef unsigned long  uint32;

	typedef unsigned long  ulong;

	typedef unsigned char  bool;

	typedef signed char    int8;
	typedef signed short   int16;
	typedef signed long    int32;

#endif
