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

#include "osapi.h"
#include "espmissingincludes.h"

#include "ets_sys.h"
#include "httpd.h"
#include "io.h"
#include "dht.h"
#include "httpdespfs.h"
#include "web.h"
#include "webwifi.h"
#include "wifi.h"
#include "action.h"
#include "config.h"

HttpdBuiltInUrl builtInUrls[]={
	{"/", httpd_cgi_redirect, "/index.tpl"},
	{"/dht22.tpl", httpdespfs_template, web_tpl_dht},
	{"/index.tpl", httpdespfs_template, web_tpl_index},
	{"/settings.tpl", httpdespfs_template, web_tpl_settings},
	{"/relayconfig.tpl", httpdespfs_template, web_tpl_relay_config},
	{"/relayconfig.cgi", web_cgi_relay_config, NULL},
	{"/relay.tpl", httpdespfs_template, web_tpl_relay},
	{"/relay.cgi", web_cgi_relay, NULL},

	//Routines to make the /wifi URL and everything beneath it work.
	{"/wifi", httpd_cgi_redirect, "/wifi/wifi.tpl"},
	{"/wifi/", httpd_cgi_redirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", webwifi_cgi_scan, NULL},
	{"/wifi/wifi.tpl", httpdespfs_template, webwifi_tpl},
	{"/wifi/connect.cgi", webwifi_cgi_connect},
	{"/wifi/setmode.cgi", webwifi_cgi_set_mode, NULL},

	{"*", httpdespfs_hook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


void user_init(void) {
	io_init();
	dht_init(SENSORTYPE, POOLTIME);
	httpd_init(builtInUrls, 80);
        wifi_init();
	config_init();	
        action_init();
	os_printf("\nESP Ready\n");
}

void user_rf_pre_init(void)
{
}
