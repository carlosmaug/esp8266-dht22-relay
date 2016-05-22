/****************************************************************************
 * Copyright (C) 2016 by Carlos Martin Ugalde and Ignacio Ripoll García     *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/**
 * @file action.c
 * @author Carlos Martin Ugalde and Ignacio Ripoll García
 * @date 15 May 2016
 * @brief File containing basic itoa function.
 *
 * Ralay is turned on and off depending on DHT readins and the parameters set 
 * into the configuration. SLEEP_TIME Defines the time between sensor readings.
 */

char* itoa(int i, char b[]) { 
	char const digit[] = "0123456789";
	char* p = b;

	if (i < 0) {
		*p++ = '-';
		i *= -1;
	}
	
	int shifter = i;

	//Move to where representation ends
	do { 
		++p;
		shifter = shifter/10;
	} while (shifter);

	*p = '\0';

	//Move back, inserting digits as you go
	do {
		*--p = digit[i%10];
		i = i/10;
	} while (i);

	return b;
}
