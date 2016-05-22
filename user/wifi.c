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
 * @file wifi.c
 * @author Carlos Martin Ugalde and Ignacio Ripoll García
 * @date 15 May 2016
 * @brief File containing WIFI daemon.
 *
 * Init wifi connnecton. If we cannot connet to AP return to AM MODE for safety. 
 * 
 */

#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "espmissingincludes.h"
#include "wifi.h"

/**
 * @brief Go into AP mode if we cant connect as STATION.
 *
 */

static void ICACHE_FLASH_ATTR _wifi_check_cb(void *arg) {
	int mode = wifi_get_opmode();
        
        int conn = wifi_station_get_connect_status();
        
	// If we are in Station mode, we do this to prevent loosing WIFI access to esp
        if (mode == 1) {
		if (conn == STATION_GOT_IP) {
            		os_printf("WiFi mode: %d - got IP %d \n", mode, STATION_GOT_IP);
        	} else {
            		os_printf("Connect fail. Going into AP mode.\n");
	    		wifi_set_opmode(2);
		}
        } else if (!mode) {
		wifi_set_opmode(2);
	}
}

/**
 * @brief Init WIFI connection.
 *
 */

void ICACHE_FLASH_ATTR wifi_init(void){
    static ETSTimer wifiTimer;
    os_timer_disarm(&wifiTimer);
    os_timer_setfn(&wifiTimer, _wifi_check_cb, NULL);
    os_timer_arm(&wifiTimer, 5000, 0);
}
