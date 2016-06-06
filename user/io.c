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
 * @brief This files controls IO port connected to the realy. 
 *
 */


#include <esp8266.h>

//#include "osapi.h"
//#include "espmissingincludes.h"
//#include "user_interface.h"
//#include "gpio.h"

#include "io.h"
#include "config.h"

#define REALYGPIO 0 

void _io_off (void* arg);
static int status = 0;
static ETSTimer ioOffTimer;

/*
 * @brief Sets I/O port on/off. 
 *
 */

void ICACHE_FLASH_ATTR io_enable(short int ena) {
	if (ena) {
		GPIO_OUTPUT_SET(REALYGPIO, 1);
		status = 1;
	} else {
		GPIO_OUTPUT_SET(REALYGPIO, 0);
		status = 0;
	}
}

/*
 * @brief Restuns I/O port status. 
 *
 */

int ICACHE_FLASH_ATTR io_get_status() {
	return status;
}

/*
 * @brief I/O port init. 
 *
 */

void io_init() {
	//Set GPIO0 to output mode.
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO0);
	GPIO_OUTPUT_SET(REALYGPIO, 0);
	status = 0;
}

/*
 * @brief Sets I/O port off after a certain time. 
 *
 * Reads configuration from memory and sets a timer to turn off the I/O port 
 * after the given number of seconds.
 *
 */

void ICACHE_FLASH_ATTR io_timer(short int enable) {
	struct config conf;
	conf = config_read();	

	if (enable) {
		os_timer_disarm(&ioOffTimer);
		os_timer_setfn(&ioOffTimer, _io_off, NULL);
		os_timer_arm(&ioOffTimer, conf.time*60000, 0);
	} else {
		os_timer_disarm(&ioOffTimer);
	}
}

/*
 * @brief Dummy helper function. 
 */

void ICACHE_FLASH_ATTR _io_off (void* arg) {
	io_enable(0);
}
