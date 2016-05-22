/****************************************************************************
 * Copyright (C) by Jeroen Domburg <jeroen@spritesmods.com>                 *
 *                                                                          *
 * As long as you retain this notice you can do whatever you want with this *
 * stuff. If we meet some day, and you think this stuff is worth it, you    *
 * can buy me a beer in return.                                             *
 ****************************************************************************/

/**
 * @file webwifi.c
 * @author Jeroen Domburg <jeroen@spritesmods.com>
 * @contributors Carlos Martin Ugalde and Ignacio Ripoll García
 * @brief This file controls /wifi/wifi.tpl URL.
 *
 * 
 * This template lets you choose between three diffenert wireless modes: AP,
 * AP + client and station. If the two last modes are selected, it shows the
 * list of all SSID reachabe and the signal power of each one. 
 */

#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "web.h"
#include "io.h"
#include "espmissingincludes.h"

//WiFi access point data
typedef struct {
	char ssid[32];
	char rssi;
	char enc;
} ApData;

//Scan result
typedef struct {
	char scanInProgress;
	ApData **apData;
	int noAps;
} ScanResultData;

//Static scan status storage.
ScanResultData cgiWifiAps;

//Temp store for new ap info.
static struct station_config stconf;

/**
 * @brief Displays wifi.tpl.
 *
 * This template shows the current WIFI mode and other data. Lets us change the
 * WIFI mode and AP connection. 
 */

void ICACHE_FLASH_ATTR webwifi_tpl(HttpdConnData *connData, char *token, void **arg) {
	char buff[1024];
	int x;
	static struct station_config stconf;

	if (token == NULL) return;

	wifi_station_get_config(&stconf);

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "WiFiMode") == 0) {
		x=wifi_get_opmode();
		if (x == 1) os_strcpy(buff, "Client");
		if (x == 2) os_strcpy(buff, "AP only");
		if (x == 3) os_strcpy(buff, "Client + AP");
	} else if (os_strcmp(token, "currSsid") == 0) {
		os_strcpy(buff, (char*)stconf.ssid);
	} else if (os_strcmp(token, "WiFiPasswd") == 0) {
		os_strcpy(buff, (char*)stconf.password);
	}

	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

/**
 * @brief CGI used by wifi.tpl
 *
 * This cgi change to a specific WIFI mode: AP, AP + CLIENT, CLIENT only.
 */

int ICACHE_FLASH_ATTR webwifi_cgi_set_mode(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len = httpd_find_arg(connData->getArgs, "mode", buff, sizeof(buff));

	if (len != 0) {
		os_printf("Changing WIFI mode to: %s\n", buff);
		wifi_set_opmode(atoi(buff));
		system_restart();
	}

	httpd_redirect(connData, "/wifi");
	return HTTPD_CGI_DONE;
}

/**
 * @brief Scans all AP in range.
 *
 * It's celled each time a wlan scan is done, it stores all AP data found into 
 * the cgiWifiAps struct. 
 */

void ICACHE_FLASH_ATTR _webwifi_scan_done_cb(void *arg, STATUS status) {
	int counter;
	struct bss_info *bss_link = (struct bss_info *)arg;
	os_printf("wifiScanDoneCb %d\n", status);

	if (status != OK) {
		cgiWifiAps.scanInProgress = 0;
		wifi_station_disconnect(); //test HACK
		return;
	}

	//Clear previous ap data if needed.
	if (cgiWifiAps.apData != NULL) {
		for (counter = 0; counter < cgiWifiAps.noAps; counter++) os_free(cgiWifiAps.apData[counter]);
		os_free(cgiWifiAps.apData);
	}

	//Count amount of access points found.
	counter = 0;

	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		counter++;
	}

	//Allocate memory for access point data
	cgiWifiAps.apData = (ApData **)os_malloc(sizeof(ApData *)*counter);
	cgiWifiAps.noAps = counter;

	//Copy access point data to the static struct
	bss_link = (struct bss_info *)arg;
	counter = 0;

	while (bss_link != NULL) {
		cgiWifiAps.apData[counter] = (ApData *)os_malloc(sizeof(ApData));
		cgiWifiAps.apData[counter]->rssi = bss_link->rssi;
		cgiWifiAps.apData[counter]->enc = bss_link->authmode;
		strncpy(cgiWifiAps.apData[counter]->ssid, (char*)bss_link->ssid, 32);

		bss_link = bss_link->next.stqe_next;
		counter++;
	}

	os_printf("Scan done: found %d APs\n", counter);
	//We're done.
	cgiWifiAps.scanInProgress = 0;
}

/**
 * @brief Prints WIFI connection status to console.
 */

static void ICACHE_FLASH_ATTR _print_wifi_status(uint8 station_status) {
    os_printf("Station status: ");

    switch(station_status) {
        case STATION_IDLE:
       	    os_printf("IDLE");
            break;

        case STATION_CONNECTING:
             os_printf("CONNECTING");
             break;

        case STATION_WRONG_PASSWORD:
             os_printf("WRONG_PASSWORD");
             break;

        case STATION_NO_AP_FOUND:
             os_printf("NO_AP_FOUND");
             break;

        case STATION_CONNECT_FAIL:
             os_printf("CONNECT_FAIL");
             break;
        case STATION_GOT_IP:
             os_printf("GOT_IP");
             break;
    }

    os_printf("\n");
}

/**
 * @brief Start AP scan.
 */

static void ICACHE_FLASH_ATTR _webwifi_start_scan() {
	int conn;
	cgiWifiAps.scanInProgress=1;
	conn = wifi_station_get_connect_status();
    
        os_printf("Starting AP Scan...\n");
        _print_wifi_status(conn);

	if (conn != STATION_GOT_IP) {
		//Unit probably is trying to connect to a bogus AP. This messes up scanning. Stop that.
		os_printf("WIFI connection status = %d. Disconnecting...\n", conn);
		wifi_station_disconnect();
	}

	wifi_station_scan(NULL, _webwifi_scan_done_cb);
}

/**
 * @brief Send collected AP data to wifi.tpl.
 *
 * This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
 * scan for access points and if available will return the result of an earlier
 * scan. The result is embedded in a bit of JSON parsed by the javascript in 
 * wifi.tpl.
 */

int ICACHE_FLASH_ATTR webwifi_cgi_scan(HttpdConnData *connData) {
	int len;
	int i;
	char buff[1024];
	httpd_start_response(connData, 200);
	httpd_header(connData, "Content-Type", "text/json");
	httpd_end_headers(connData);

	if (cgiWifiAps.scanInProgress == 1) {
		os_printf("{\"result\": { \"inProgress\": \"1\" } }\n");
		len = os_sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
		espconn_sent(connData->conn, (uint8 *)buff, len);
	} else {
		os_printf("{\"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
		len = os_sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
		espconn_sent(connData->conn, (uint8 *)buff, len);

		if (cgiWifiAps.apData == NULL) cgiWifiAps.noAps = 0;

		for (i = 0; i < cgiWifiAps.noAps; i++) {
			os_printf("{\"essid\": \"%s\", \"rssi\": \"%d\", \"enc\": \"%d\"}%s\n", 
					cgiWifiAps.apData[i]->ssid, cgiWifiAps.apData[i]->rssi, 
					cgiWifiAps.apData[i]->enc, (i == cgiWifiAps.noAps-1)?"":",");
			len = os_sprintf(buff, "{\"essid\": \"%s\", \"rssi\": \"%d\", \"enc\": \"%d\"}%s\n", 
					cgiWifiAps.apData[i]->ssid, cgiWifiAps.apData[i]->rssi, 
					cgiWifiAps.apData[i]->enc, (i == cgiWifiAps.noAps-1)?"":",");
			espconn_sent(connData->conn, (uint8 *)buff, len);
		}

		os_printf("]\n}\n}\n");
		len = os_sprintf(buff, "]\n}\n}\n");
		espconn_sent(connData->conn, (uint8 *)buff, len);
		_webwifi_start_scan();
	}

	return HTTPD_CGI_DONE;
}

/**
 * @brief Check if we are porperly connected to AP
 *
 * This routine is ran some time after a connection attempt to an AP.
 * If the connection succeeds, ESP is reset to go into the selected mode.
 */

static void ICACHE_FLASH_ATTR _reset_timer_cb(void *arg) {
	int conn = wifi_station_get_connect_status();
        os_printf("WiFi State Update: ");
        _print_wifi_status(conn);

	if (conn == STATION_GOT_IP) {
		//Go to STA mode. This needs a reset, so do that.
		wifi_set_opmode(1);
		system_restart();
	} else {
		os_printf("Connect fail. Not going into STA-only mode.\n");
	}
}

/**
 * @brief Connected to AP
 *
 * This routine is timed because I had problems with immediate connections.
 * It probably was something else that caused it, but I did'n whant to put
 * the code back :P.
 */

static void ICACHE_FLASH_ATTR _reass_timer_cb(void *arg) {
	static ETSTimer resetTimer;
	wifi_station_disconnect();
	wifi_station_set_config(&stconf);
	os_printf("Connecting to %s with password %s", stconf.ssid, stconf.password);
	wifi_station_connect();

	if (wifi_get_opmode() != 1) {
		//Schedule disconnect/connect
		os_timer_disarm(&resetTimer);
		os_timer_setfn(&resetTimer, _reset_timer_cb, NULL);
		os_timer_arm(&resetTimer, 4000, 0);
	}
}

/**
 * @brief CGI used by wifi.tpl
 *
 * This cgi connect to a specific access point with the given ESSID using the
 * given password.
 */

int ICACHE_FLASH_ATTR webwifi_cgi_connect(HttpdConnData *connData) {
	char essid[128];
	char passwd[128];
	static ETSTimer reassTimer;
	
	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}
	
	httpd_find_arg(connData->postBuff, "essid", essid, sizeof(essid));
	httpd_find_arg(connData->postBuff, "passwd", passwd, sizeof(passwd));

	os_strncpy((char*)stconf.ssid, essid, 32);
	os_strncpy((char*)stconf.password, passwd, 64);
    
	os_printf("Will connect to %s using password %s", essid, passwd);

	//Schedule disconnect/connect
	os_timer_disarm(&reassTimer);
	os_timer_setfn(&reassTimer, _reass_timer_cb, NULL);
	os_timer_arm(&reassTimer, 1000, 0);
	httpd_redirect(connData, "/wifi/connecting.html");
	return HTTPD_CGI_DONE;
}
