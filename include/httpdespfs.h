#ifndef HTTPDESPFS_H
#define HTTPDESPFS_H

#include "httpd.h"
#include "espfs.h"

int httpdespfs_hook(HttpdConnData *connData);
int ICACHE_FLASH_ATTR httpdespfs_template(HttpdConnData *connData);

#endif
