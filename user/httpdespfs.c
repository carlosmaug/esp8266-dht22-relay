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
 * @brief Connector to let httpd use the espfs filesystem to serve the files in it. 
 *
 */

#include "espmissingincludes.h"
#include <string.h>
#include <osapi.h>
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include "httpd.h"
#include "espfs.h"
#include "httpdespfs.h"

typedef struct {
	EspFsFile *file;
	void *tplArg;
	char token[64];
	int tokenPos;
} TplData;

typedef void (*TplCallback)(HttpdConnData *connData, char *token, void **arg);


/*
 * @brief Catch-all cgi function. 
 *
 * It takes the url passed to it, looks up the corresponding path in the 
 * filesystem and if it exists, passes the file through. This simulates what a
 * normal webserver would do with static files.
 */

int ICACHE_FLASH_ATTR httpdespfs_hook(HttpdConnData *connData) {
	int len;
	char buff[1024];

	EspFsFile *file = connData->cgiData;

	if (connData->conn == NULL) {
		// Connection aborted. Clean up.
		espfs_close(file);
		return HTTPD_CGI_DONE;
	}

	if (file == NULL) {
		// First call to this cgi. Open the file so we can read it.
		file = espfs_open(connData->url);

		if (file == NULL) {
			return HTTPD_CGI_NOTFOUND;
		}

		connData->cgiData = file;
		httpd_start_response(connData, 200);
		httpd_header(connData, "Content-Type", httpd_get_mimetype(connData->url));
		httpd_end_headers(connData);
		return HTTPD_CGI_MORE;
	}

	len = espfs_read(file, buff, 1024);

	if (len > 0) { 
		espconn_sent(connData->conn, (uint8 *)buff, len);
	}

	if (len != 1024) {
		// We're done.
		espfs_close(file);
		return HTTPD_CGI_DONE;
	} else {
		// Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}


/*
 * @brief this function can be used to parse templates.
 * 
 */

int ICACHE_FLASH_ATTR httpdespfs_template(HttpdConnData *connData) {
	int len;
	int x, sp = 0;
	char *e = NULL;
	char buff[1025];

	TplData *tpd = connData->cgiData;

	if (connData->conn == NULL) {
		// Connection aborted. Clean up.
		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		espfs_close(tpd->file);
		os_free(tpd);
		return HTTPD_CGI_DONE;
	}

	if (tpd == NULL) {
		// First call to this cgi. Open the file so we can read it.
		tpd = (TplData *)os_malloc(sizeof(TplData));
		tpd->file = espfs_open(connData->url);
		tpd->tplArg = NULL;
		tpd->tokenPos = -1;

		if (tpd->file == NULL) {
			return HTTPD_CGI_NOTFOUND;
		}

		connData->cgiData = tpd;
		httpd_start_response(connData, 200);
		httpd_header(connData, "Content-Type", httpd_get_mimetype(connData->url));
		httpd_end_headers(connData);
		return HTTPD_CGI_MORE;
	}

	len = espfs_read(tpd->file, buff, 1024);
	if (len > 0) {
		sp = 0;
		e  = buff;

		for (x=0; x < len; x++) {
			if (tpd->tokenPos == -1) {
				// Inside ordinary text.
				if (buff[x] == '%') {
					// Send raw data up to now
					if (sp != 0) {
						espconn_sent(connData->conn, (uint8 *)e, sp);
					}

					sp = 0;

					// Go collect token chars.
					tpd->tokenPos = 0;
				} else {
					sp++;
				}
			} else {
				if (buff[x] == '%') {
					// Zero-terminate token
					tpd->token[tpd->tokenPos++] = 0; 
					((TplCallback)(connData->cgiArg))(connData, tpd->token, &tpd->tplArg);

					// Go collect normal chars again.
					e = &buff[x+1];
					tpd->tokenPos = -1;
				} else {
					if (tpd->tokenPos < (sizeof(tpd->token)-1)) {
						tpd->token[tpd->tokenPos++] = buff[x];
					}
				}
			}
		}
	}

	// Send remaining bit.
	if (sp != 0) {
		espconn_sent(connData->conn, (uint8 *)e, sp);
	}

	if (len != 1024) {
		// We're done.
		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
		espfs_close(tpd->file);
		return HTTPD_CGI_DONE;
	} else {
		// Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}

