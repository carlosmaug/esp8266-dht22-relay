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
 * @brief Esp8266 http server - core routines 
 *
 */

#include <osapi.h>

#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include "espconn.h"
#include "httpd.h"
#include "io.h"
#include "espfs.h"

// Max length of request head
#define MAX_HEAD_LEN 1024

// Max amount of connections
#define MAX_CONN 8

// Max post buffer len
#define MAX_POST 1024

// Debug mode 1 = on
#define DEBUG 0 

// This gets set at init time.
static HttpdBuiltInUrl *builtInUrls;

// Private data for httpd thing
struct HttpdPriv {
	char head[MAX_HEAD_LEN];
	int headPos;
	int postPos;
};

// Connection pool
static HttpdPriv connPrivData[MAX_CONN];
static HttpdConnData connData[MAX_CONN];

static struct espconn httpdConn;
static esp_tcp httpdTcp;

static const char *httpNotFoundHeader = "HTTP/1.0 404 Not Found\r\nServer: esp8266-httpd/0.1\r\nContent-Type: text/plain\r\n\r\nNot Found.\r\n";

typedef struct {
	const char *ext;
	const char *mimetype;
} MimeMap;

/*
 * The mappings from file extensions to mime types. If you need an extra mime
 * type, add it here.
 */

static const MimeMap mimeTypes[]={
	{"htm", "text/htm"},
	{"html", "text/html"},
	{"js", "text/javascript"},
	{"txt", "text/plain"},
	{"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},
	{"png", "image/png"},
	{NULL, "text/html"}, //default value
};

/*
 * @brief Returns a static char* to a mime type for a given url to a file. 
 */

const char ICACHE_FLASH_ATTR *httpd_get_mimetype(char *url) {
	int i = 0;
	// Go find the extension
	char *ext = url+(strlen(url)-1);

	while (ext != url && *ext != '.') ext--;

	if (*ext == '.') ext++;

	while (mimeTypes[i].ext != NULL && os_strcmp(ext, mimeTypes[i].ext)!=0) i++;

	return mimeTypes[i].mimetype;
}

/*
 * @brief Looks up the connData info for a specific esp connection 
 */

static HttpdConnData ICACHE_FLASH_ATTR *_httpd_find_conn_data(void *arg) {
	int i;

	for (i = 0; i < MAX_CONN; i++) {
		if (connData[i].conn == (struct espconn *)arg) return &connData[i];
	}

#if DEBUG
	os_printf("FindConnData: Huh? Couldn't find connection for %p\n", arg);
#endif
	return NULL; // WtF?
}

/*
 * @brief Clean up a connection for re-use 
 */

static void ICACHE_FLASH_ATTR _httpd_retire_conn(HttpdConnData *connData) {

	if (connData->postBuff != NULL) os_free(connData->postBuff);

	connData->postBuff = NULL;
	connData->cgi      = NULL;
	connData->conn     = NULL;
}

/*
 * @brief Helper function that returns the value of a hex char.
 */

static int _httpd_hex_val(char c) {

	if (c >= '0' && c <= '9') return c-'0';

	if (c >= 'A' && c <= 'F') return c-'A'+10;

	if (c >= 'a' && c <= 'f') return c-'a'+10;

	return 0;
}

/*
 * @brief Decode a percent-encoded value.
 *
 * Takes the 'valLen' bytes stored in 'val', and converts it into at most 
 * 'retLen' bytes that are stored in the 'ret' buffer. Returns the actual amount
 * of bytes used in 'ret', or uses zero to terminate the 'ret' buffer.
 */

int httpd_url_decode(char *val, int valLen, char *ret, int retLen) {
	int s = 0, d = 0;
	int esced = 0, escVal = 0;

	while (s < valLen && d < retLen) {
		if (esced == 1)  {
			escVal = _httpd_hex_val(val[s]) << 4;
			esced = 2;
		} else if (esced == 2) {
			escVal += _httpd_hex_val(val[s]);
			ret[d++] = escVal;
			esced = 0;
		} else if (val[s] == '%') {
			esced = 1;
		} else if (val[s] == '+') {
			ret[d++] = ' ';
		} else {
			ret[d++] = val[s];
		}

		s++;
	}

	if (d < retLen) ret[d] = 0;

	return d;
}

/*
 * @brief Find a specific arg in a string of get or post data.
 *
 * Line is the string of post/get-data, arg is the name of the value to find. 
 * The zero-terminated result is written in buff, with at most buffLen bytes
 * used. The function returns the length of the result, or -1 if the value
 * wasn't found.
 */

int ICACHE_FLASH_ATTR httpd_find_arg(char *line, char *arg, char *buff, int buffLen) {
	char *p, *e;

	if (line == NULL) return 0;

	p = line;

	while(p != NULL && *p != '\n' && *p != '\r' && *p != 0) {
#if DEBUG
		os_printf("httpd_find_arg: %s\n", p);
#endif
		if (os_strncmp(p, arg, os_strlen(arg)) == 0 && p[strlen(arg)] == '=') {
			p += os_strlen(arg) + 1; // Move p to start of value
			e = (char*)os_strstr(p, "&");

			if (e == NULL) e = p + os_strlen(p);

#if DEBUG
			os_printf("httpd_find_arg: val %s len %d\n", p, (e-p));
#endif
			return httpd_url_decode(p, (e-p), buff, buffLen);
		}

		p = (char*)os_strstr(p, "&");

		if (p != NULL) p++;
	}

#if DEBUG
	os_printf("httpd_find_arg: Finding %s in %s: Not found :/\n", arg, line);
#endif
	return -1; // Not found
}

/*
 * @brief Start the response headers.
 */
 
void ICACHE_FLASH_ATTR httpd_start_response(HttpdConnData *connData, int code) {
	char buff[128];
	int l;

	l = os_sprintf(buff, "HTTP/1.0 %d OK\r\nServer: esp8266-httpd/0.1\r\n", code);
	espconn_sent(connData->conn, (uint8 *)buff, l);
}

/*
 * @brief Send a http header.
 */
 
void ICACHE_FLASH_ATTR httpd_header(HttpdConnData *connData, const char *field, const char *val) {
	char buff[256];
	int l;

	l = os_sprintf(buff, "%s: %s\r\n", field, val);
	espconn_sent(connData->conn, (uint8 *)buff, l);
}

/*
 * @brief Finish the headers.
 */
 
void ICACHE_FLASH_ATTR httpd_end_headers(HttpdConnData *connData) {
	espconn_sent(connData->conn, (uint8 *)"\r\n", 2);
}

/*
 * @brief Implementation of REDIRECT of HTTP protocol
 *
 * As we said in the readme file snprintf sould be used instead of sprintf to 
 * prevent buffer overflows. 
 *
 * At the moment snprintf is not jet implemented for the ESP8266 sdk
 */

// ToDo: sprintf->snprintf everywhere
void ICACHE_FLASH_ATTR httpd_redirect(HttpdConnData *connData, char *newUrl) {
	char buff[1024];
	int l;
	l = os_sprintf(buff, "HTTP/1.1 302 Found\r\nLocation: %s\r\n\r\nMoved to %s\r\n", newUrl, newUrl);
	espconn_sent(connData->conn, (uint8 *)buff, l);
}

/*
 * @brief Extends httpd_redirect, by checking if there is a connection opened. 
 *
 */

 int ICACHE_FLASH_ATTR httpd_cgi_redirect(HttpdConnData *connData) {
	if (connData->conn == NULL) {
		// Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	httpd_redirect(connData, (char*)connData->cgiArg);
	return HTTPD_CGI_DONE;
}

/*
 * @brief Function which will be called back when data are successfully sent. 
 *
 */

static void ICACHE_FLASH_ATTR _httpd_sent_cb(void *arg) {
	int r;
	HttpdConnData *connData = _httpd_find_conn_data(arg);
#if DEBUG
	os_printf("Sent callback on conn %p\n", conn);
#endif

	// Connection aborted. Clean up.
	if (connData == NULL) return;

	// Marked for destruction?	
	if (connData->cgi == NULL) { 
#if DEBUG
		os_printf("Conn %p is done. Closing.\n", connData->conn);
#endif
		espconn_disconnect(connData->conn);
		_httpd_retire_conn(connData);
		return;
	}

	// Execute cgi fn.
	r = connData->cgi(connData); 

	if (r == HTTPD_CGI_DONE) {
		// Mark for destruction.	
		connData->cgi = NULL; 
	}
}

/*
 * @brief Sends an HTTP response to a valid HTTP request. 
 *
 * Looks for a valid URL into builtInUrls
 */

static void ICACHE_FLASH_ATTR httpdSendResp(HttpdConnData *connData) {
	int i = 0;
	int r;

	// See if the url is somewhere in our internal url table.
	while (builtInUrls[i].url != NULL && connData->url != NULL) {
#if DEBUG
		os_printf("%s == %s?\n", builtInUrls[i].url, connData->url);
#endif
		if (os_strcmp(builtInUrls[i].url, connData->url) == 0 || builtInUrls[i].url[0] == '*') {
#if DEBUG
			os_printf("Is url index %d\n", i);
#endif
			connData->cgiData = NULL;
			connData->cgi     = builtInUrls[i].cgiCb;
			connData->cgiArg  = builtInUrls[i].cgiArg;
			r = connData->cgi(connData);

			if (r != HTTPD_CGI_NOTFOUND) {
				// If cgi finishes immediately: mark connData for destruction.
				if (r == HTTPD_CGI_DONE) connData->cgi = NULL; 

				return;
			}
		}

		i++;
	}

	// Can't find :/
	os_printf("%s not found. 404!\n", connData->url);
	espconn_sent(connData->conn, (uint8 *)httpNotFoundHeader, os_strlen(httpNotFoundHeader));
	// Mark for destruction
	connData->cgi = NULL; 
}

/*
 * @brief Parses HTTP received headers. 
 *
 */

static void ICACHE_FLASH_ATTR _httpd_parse_header(char *h, HttpdConnData *connData) {
	int i;
#if DEBUG
	os_printf("Got header %s\n", h);
#endif

	if (os_strncmp(h, "GET ", 4) == 0 || os_strncmp(h, "POST ", 5) == 0) {
		char *e;
		
		// Skip past the space after POST/GET
		i = 0;

		while (h[i] != ' ') i++;

		connData->url = h+i+1;

		// Figure out end of url.
		e = (char*)os_strstr(connData->url, " ");

		if (e == NULL) return; //wtf?

	 	// Terminate url part
		*e = 0;

		os_printf("URL = %s\n", connData->url);
		connData->getArgs = (char*)os_strstr(connData->url, "?");

		if (connData->getArgs != 0) {
			*connData->getArgs = 0;
			connData->getArgs++;
			os_printf("GET args = %s\n", connData->getArgs);
		} else {
			connData->getArgs = NULL;
		}
	} else if (os_strncmp(h, "Content-Length: ", 16)==0) {
		i = 0;

		while (h[i]!=' ') i++;

		connData->postLen = atoi(h+i+1);

		if (connData->postLen > MAX_POST) connData->postLen = MAX_POST;

#if DEBUG
		os_printf("Mallocced buffer for %d bytes of post data.\n", conn->postLen);
#endif

		connData->postBuff = (char*)os_malloc(connData->postLen+1);
		connData->priv->postPos = 0;
	}
}

/*
 * @brief This function will be called back when data are received. 
 *
 */

static void ICACHE_FLASH_ATTR _httpd_recv_cb(void *arg, char *data, unsigned short len) {
	int x;
	char *p, *e;
	HttpdConnData *connData=_httpd_find_conn_data(arg);

	if (connData == NULL) return;

	for (x = 0; x < len; x++) {

		if (connData->priv->headPos != -1) {
			// This byte is a header byte.
			if (connData->priv->headPos != MAX_HEAD_LEN) connData->priv->head[connData->priv->headPos++] = data[x];

			connData->priv->head[connData->priv->headPos] = 0;

			// Scan for /r/n/r/n
			if (data[x] == '\n' && (char *)os_strstr(connData->priv->head, "\r\n\r\n") != NULL) {
				// Reset url data
				connData->url = NULL;

				// Find end of next header line
				p = connData->priv->head;

				while(p < (&connData->priv->head[connData->priv->headPos-4])) {
					e = (char *)os_strstr(p, "\r\n");

					if (e == NULL) break;

					e[0] = 0;
					_httpd_parse_header(p, connData);
					p = e+2;
				}

				// If we don't need to receive post data, we can send the response now.
				if (connData->postLen == 0) {
					httpdSendResp(connData);
				}

				// Indicate we're done with the headers.
				connData->priv->headPos = -1; 
			}
		} else if (connData->priv->postPos !=- 1 && connData->postLen != 0 && connData->priv->postPos <= connData->postLen) {
			// This byte is a POST byte.
			connData->postBuff[connData->priv->postPos++] = data[x];

			if (connData->priv->postPos >= connData->postLen) {
				// Received post stuff.
				connData->postBuff[connData->priv->postPos] = 0; //zero-terminate
				connData->priv->postPos= -1;
				os_printf("Post data: %s\n", connData->postBuff);

				// Send the response.
				httpdSendResp(connData);
				return;
			}
		}
	}
}

/*
 * @brief This function will be called back when TCP reconnection starts. 
 *
 * It seems that its purpose is to register a callback when an error in the 
 * TCP/IP calls or results is detected. This appears to be nothing to do with
 * "reconnect" but rather with error handling. 
 *
 * We just print out that there has been an error in the connection.
 */

static void ICACHE_FLASH_ATTR _httpd_recon_cb(void *arg, sint8 err) {
	os_printf("TCP reconnection.\n");
}

/* 
 * @brief This function will be called back under successful TCP disconnection.
 *
 */

static void ICACHE_FLASH_ATTR _httpd_discon_cb(void *arg) {
	// Just look at all the sockets and kill the slot if needed.
	int i;

	for (i = 0; i < MAX_CONN; i++) {
		if (connData[i].conn != NULL) {
			if (connData[i].conn->state == ESPCONN_NONE || connData[i].conn->state == ESPCONN_CLOSE) {
				connData[i].conn = NULL;

			 	// Flush cgi data	
				if (connData[i].cgi != NULL) connData[i].cgi(&connData[i]);

				_httpd_retire_conn(&connData[i]);
			}
		}
	}
}

/* 
 * @brief This function will be called when TCP reconnection starts.
 *
 */

static void ICACHE_FLASH_ATTR _httpd_connect_cb(void *arg) {
	struct espconn *conn = arg;
	int i;

	// Find empty conndata in pool
	for (i = 0; i < MAX_CONN; i++) if (connData[i].conn == NULL) break;

#if DEBUG
	os_printf("Con req, conn=%p, pool slot %d\n", conn, i);
#endif
	connData[i].priv = &connPrivData[i];

	if (i == MAX_CONN) {
		os_printf("Aiee, conn pool overflow!\n");
		espconn_disconnect(conn);
		return;
	}

	connData[i].conn 	  = conn;
	connData[i].priv->headPos = 0;
	connData[i].postBuff 	  = NULL;
	connData[i].priv->postPos = 0;
	connData[i].postLen 	  = 0;

	espconn_regist_recvcb(conn, _httpd_recv_cb);
	espconn_regist_reconcb(conn, _httpd_recon_cb);
	espconn_regist_disconcb(conn, _httpd_discon_cb);
	espconn_regist_sentcb(conn, _httpd_sent_cb);
}


/* 
 * @brief This function will be called at ESP start.
 *
 */

void ICACHE_FLASH_ATTR httpd_init(HttpdBuiltInUrl *fixedUrls, int port) {
	int i;

	for (i = 0; i < MAX_CONN; i++) {
		connData[i].conn = NULL;
	}

	httpdConn.type 	    = ESPCONN_TCP;
	httpdConn.state     = ESPCONN_NONE;
	httpdTcp.local_port = port;
	httpdConn.proto.tcp = &httpdTcp;
	builtInUrls	    = fixedUrls;

	os_printf("Httpd init, conn=%p\n", &httpdConn);
	espconn_regist_connectcb(&httpdConn, _httpd_connect_cb);
	espconn_accept(&httpdConn);
}
