/****************************************************************************
 * Copyright (C) by Jeroen Domburg <jeroen@spritesmods.com>                 *
 *                                                                          *
 * As long as you retain this notice you can do whatever you want with this *
 * stuff. If we meet some day, and you think this stuff is worth it, you    *
 * can buy me a beer in return.                                             *
 ****************************************************************************/

/**
 * @file espfs.c
 * @author Jeroen Domburg <jeroen@spritesmods.com>
 * @contributors Carlos Martin Ugalde and Ignacio Ripoll García
 * @brief ESP init function. 
 *
 */

#include <esp8266.h> 
#include <httpd.h> 
#include <httpdespfs.h> 
#include "webpages-espfs.h"
#include <espfs.h> 

#include "ets_sys.h"
#include "io.h"
#include "dht.h"
#include "web.h"
#include "webwifi.h"
#include "wifi.h"
#include "action.h"
#include "config.h"
#include "stdout.h"

HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.tpl"},
	{"/index.tpl", cgiEspFsTemplate, web_tpl_index},
	{"/settings.tpl", cgiEspFsTemplate, web_tpl_settings},
	{"/relayconfig.tpl", cgiEspFsTemplate, web_tpl_relay_config},
	{"/relayconfig.cgi", web_cgi_relay_config, NULL},
	{"/relay.cgi", web_cgi_relay, NULL},

	//Routines to make the /wifi URL and everything beneath it work.
	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", webwifi_cgi_scan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, webwifi_tpl},
	{"/wifi/connect.cgi", webwifi_cgi_connect},
	{"/wifi/setmode.cgi", webwifi_cgi_set_mode, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


void user_init(void) {
	stdout_init();
	io_init();
	dht_init(SENSORTYPE, POOLTIME);

	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif
	httpdInit(builtInUrls, 80);

        wifi_init();
	config_init();	
        action_init();
	os_printf("\nESP Ready\n");
}

void user_rf_pre_init(void)
{
}
