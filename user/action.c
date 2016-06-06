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
 * @brief File containing basic actions based on DTH readings and configuration.
 *
 * Ralay is turned on and off depending on DHT readins and the parameters set 
 * into the configuration. SLEEP_TIME Defines the time between sensor readings.
 */

#include <esp8266.h>

#include <dht.h>
#include <io.h>
#include <config.h>

/**
 * milliseconds between checks 
 */
#define SLEEP_TIME 30000  

static ETSTimer actionTimer;

static int maxReached = 0;

/**
 * @brief Turn on and off realy, based on DHT readings and configutation.
 *
 */

static void _action_task(void *arg) {
	struct config currConfig = config_read();
	struct DhtReading *r = dht_read(0);
	float temp = r->temperature;
	float hum  = r->humidity;

	if (r->success && !currConfig.off) {
		if (hum > currConfig.hum || temp > currConfig.temp) {
			if (!maxReached) {
				os_printf("Max humidity/temperature exceeded.\n");
				maxReached = 1;
        			io_enable(1);
			}
		} else if (maxReached) {
			os_printf("Humidity/temperature is back to nomal levels.\n");
			maxReached = 0;
			io_enable(0);
		}
	}
}

/**
 * @brief Sensor watchdog initialization.
 *
 */

void action_init(void) {
	struct config currConfig = config_read();
	os_printf("Initializing relay trigger Max humidity allowed: %d, Max temperature allowed: %d\n", (int)currConfig.hum, (int)currConfig.temp);
        os_timer_disarm(&actionTimer);
	os_timer_setfn(&actionTimer, _action_task, NULL);
        os_timer_arm(&actionTimer, SLEEP_TIME, 1);
}
