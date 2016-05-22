/****************************************************************************
 * Copyright (C) by Jeroen Domburg <jeroen@spritesmods.com>                 *
 *                                                                          *
 * As long as you retain this notice you can do whatever you want with this *
 * stuff. If we meet some day, and you think this stuff is worth it, you    *
 * can buy me a beer in return.                                             *
 ****************************************************************************/

/**
 * @file wifi.c
 * @author Jeroen Domburg <jeroen@spritesmods.com>
 * @contributors Carlos Martin Ugalde and Ignacio Ripoll García
 * @brief File containing all functions needed to interact with the web 
 * interface
 *
 * Each web template you want to be displayed must have a defined function in
 * this file. 
 */

#include <osapi.h>
#include "espmissingincludes.h"

#include "web.h"
#include "io.h"
#include "itoa.h"
#include "dht.h"
#include "config.h"

//Debug mode 1 = on
#define DEBUG 0 

static long hitCounter = 0;

/**
 * @brief Displays relay.tpl.
 *
 * This template shows the current state of the relay and let us turn it on and 
 * off according to the actual state.
 */

void ICACHE_FLASH_ATTR web_tpl_relay(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	int currRelayStatus = io_get_status();

	if (!token) return;

	os_strcpy(buff, "Unknown");

	if (!os_strcmp(token, "relayStatus")) {
		if (currRelayStatus) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
	} else {
		os_printf("Argument 'relayStatus' not found, check relay.tpl file.\n");
	}

# if DEBUG
	os_printf("cgi_tpl_relay Buff: %s\n", buff);
# endif

	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

/**
 * @brief Turns the realy on and off.
 *
 * It turns on and off the relay according to the data received. This CGI is
 * called by realy.tpl 
 */

int ICACHE_FLASH_ATTR web_cgi_relay(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len = httpd_find_arg(connData->getArgs, "relay", buff, sizeof(buff));
	if (len) {
		if (!os_strcmp(buff, "on")) {
			io_enable(1);
			io_timer(1);
		} else {
			io_enable(0);
			io_timer(0);
		}
	} else {
		os_printf("Argument 'relay' not found, check relay.tpl file.\n");
	}

	httpd_redirect(connData, "relay.tpl");
	return HTTPD_CGI_DONE;
}

/**
 * @brief Displays settings.tpl.
 *
 * This template gives you access to the global and wifi configuraton. 
 */

void ICACHE_FLASH_ATTR web_tpl_settings(HttpdConnData *connData, char *token, void **arg) {
	return;
}

/**
 * @brief Displays relayconfig.tpl.
 *
 * This template shows the configuration parameters used in this application. 
 * It lets you modify and save them into the ESP rom, so they are kept in case
 * of reboot.
 */

void ICACHE_FLASH_ATTR web_tpl_relay_config(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	char data[6];
	struct config conf = config_read();

	if (!token) return;

	os_strcpy(buff, "unknown");

	if (!os_strcmp(token, "status")) {
		if (conf.off) {
			os_strcpy(buff, "off");
		} else {
			os_strcpy(buff, "on");
		}
	} else if (!os_strcmp(token, "humidity")) {
		os_strcpy(buff, itoa(conf.hum, data));
	} else if (!os_strcmp(token, "temperature")) {
		os_strcpy(buff, itoa(conf.temp, data));
	} else if (!os_strcmp(token, "time")) {
		os_strcpy(buff, itoa(conf.time, data));
	}

# if DEBUG
	os_printf("cgi_tpl_relay_config buff: %s\n", token);
	os_printf("cgi_tpl_relay_config buff: %s\n", buff);
# endif

	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

/**
 * @brief Sets and saves cofiguration sent by config.tpl.
 *
 * Sets global parameters for the application and saves them into the ESP memory 
 * permanently.
 */

int ICACHE_FLASH_ATTR web_cgi_relay_config(HttpdConnData *connData) {
	int len;
	char buff[128];
	struct config conf;
	
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len = httpd_find_arg(connData->getArgs, "relay", buff, sizeof(buff));
	if (len) {
		if (!os_strcmp(buff, "on")) {
			conf.off = 0;
		} else {
			conf.off = 1;
		}
	}

	len = httpd_find_arg(connData->getArgs, "humidity", buff, sizeof(buff));
	if (len) {
		conf.hum = atoi(buff);
	}

	len = httpd_find_arg(connData->getArgs, "temperature", buff, sizeof(buff));
	if (len) {
		conf.temp = atoi(buff);
	}

	len = httpd_find_arg(connData->getArgs, "time", buff, sizeof(buff));
	if (len) {
		conf.time = atoi(buff);
	}

# if DEBUG
	os_printf("cgi_relay_config: On: %d, Hum: %d, Temp: %d, Time: %d\n", conf.off, conf.hum, conf.temp, conf.time);
# endif

	config_save(conf);

	httpd_redirect(connData, "relayconfig.tpl");
	return HTTPD_CGI_DONE;
}

/**
 * @brief Displays index.tpl.
 *
 * This template shows the main page. It has a counter to know how many times
 * it has been reqestel since last reboot. 
 */

void ICACHE_FLASH_ATTR web_tpl_index(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];

	if (!token) return;

	if (os_strcmp(token, "counter") == 0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}

	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

/**
 * @brief Displays dht22.tpl.
 *
 * This template shows the readings of the DHT sensor. 
 */

void ICACHE_FLASH_ATTR web_tpl_dht(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];

	if (!token) return;

	struct DhtReading *r = dht_read(0);
	
	os_strcpy(buff, "Unknown");

	if (os_strcmp(token, "temperature") == 0) {
		float lastTemp = r->temperature;
                int integer = (int)(lastTemp);
		int decimal = (int)((lastTemp - (int)lastTemp)*100);

		if (decimal < 0) {
			decimal = -1*decimal;
		}

		os_sprintf(buff, "%d.%d", integer, decimal);		
	} else if (os_strcmp(token, "humidity") == 0) {
		float lastHum = r->humidity;
		os_sprintf(buff, "%d.%d", (int)(lastHum),(int)((lastHum - (int)lastHum)*100) );		
	} else if (os_strcmp(token, "sensor_present") == 0) {
		os_sprintf(buff, r->success?"is":"isn't");
	}
	
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}
