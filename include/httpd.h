#ifndef HTTPD_H
#define HTTPD_H
#include <ip_addr.h>
#include <c_types.h>
#include <espconn.h>

#define HTTPD_CGI_MORE 0
#define HTTPD_CGI_DONE 1
#define HTTPD_CGI_NOTFOUND 2

typedef struct HttpdPriv HttpdPriv;
typedef struct HttpdConnData HttpdConnData;

typedef int (* cgiSendCallback)(HttpdConnData *connData);

//A struct describing a http connection. This gets passed to cgi functions.
struct HttpdConnData {
	struct espconn *conn;
	char *url;
	char *getArgs;
	const void *cgiArg;
	void *cgiData;
	HttpdPriv *priv;
	cgiSendCallback cgi;
	int postLen;
	char *postBuff;
};

//A struct describing an url. This is the main struct that's used to send different URL requests to
//different routines.
typedef struct {
	const char *url;
	cgiSendCallback cgiCb;
	const void *cgiArg;
} HttpdBuiltInUrl;

int ICACHE_FLASH_ATTR httpd_cgi_redirect(HttpdConnData *connData);
void ICACHE_FLASH_ATTR httpd_redirect(HttpdConnData *conn, char *newUrl);
int httpd_url_decode(char *val, int valLen, char *ret, int retLen);
int ICACHE_FLASH_ATTR httpd_find_arg(char *line, char *arg, char *buff, int buffLen);
void ICACHE_FLASH_ATTR httpd_init(HttpdBuiltInUrl *fixedUrls, int port);
const char *httpd_get_mimetype(char *url);
void ICACHE_FLASH_ATTR httpd_start_response(HttpdConnData *conn, int code);
void ICACHE_FLASH_ATTR httpd_end_headers(HttpdConnData *conn);
void ICACHE_FLASH_ATTR httpd_header(HttpdConnData *conn, const char *field, const char *val);

#endif
