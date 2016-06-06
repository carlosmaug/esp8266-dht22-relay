#ifndef CGI_H
#define CGI_H

#include "httpd.h"

void web_tpl_relay_config(HttpdConnData *connData, char *token, void **arg);
int  web_cgi_relay_config(HttpdConnData *connData);
int  web_cgi_relay(HttpdConnData *connData);
void web_tpl_settings(HttpdConnData *connData, char *token, void **arg);
void web_tpl_index(HttpdConnData *connData, char *token, void **arg);

#endif
