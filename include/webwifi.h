#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"

void webwifi_tpl(HttpdConnData *connData, char *token, void **arg);
int  webwifi_cgi(HttpdConnData *connData);
int  webwifi_cgi_scan(HttpdConnData *connData);
int  webwifi_cgi_connect(HttpdConnData *connData);
int  webwifi_cgi_set_mode(HttpdConnData *connData);
#endif
