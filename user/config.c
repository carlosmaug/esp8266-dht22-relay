/****************************************************************************
 * Copyright (C) 2016 by Carlos Martin Ugalde and Ignacio Ripoll García     *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/**
 * @file config.c
 * @author Carlos Martin Ugalde and Ignacio Ripoll García
 * @date 15 May 2016
 * @brief File containing configuration save/recover functions.
 *
 * Ralay is turned on and off depending on DHT readins and the parameters set 
 */

#include "ets_sys.h"
#include "osapi.h"
#include "spi_flash.h"
#include "espmissingincludes.h"
#include "config.h"
#include "mem.h"

// https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map
// Acording to this map we wave 4k free starting on 0x7B00
#define CONFIG_SECTOR 0x7B

// To write , you need the sector address. 
#define CONFIG_ADDRESS (SPI_FLASH_SEC_SIZE * CONFIG_SECTOR)

void _read(void);
int _write(struct config conf);
void _default_data(void); 

struct config confRead;

// Get saved config on startup
void config_init() {
	_read();
	os_printf("Initial config; Humidity %d, Temperature: %d, Off: %d\n", confRead.hum, confRead.temp, confRead.off);
}

// Save config and check if it is correctly stored.
int config_save(struct config save) {
	if (_write(save)) {
		return 1;
	}

	confRead = save;

	if (memcmp(&confRead, &save, sizeof(struct config)) != 0) {
		return 1;
	}

	return 0;
}

// Read config 
struct config config_read() {
	return confRead;
}

// Read stored config
void _read() {
	
	int sum1 = 0; 
	int sum2 = 0;
	short int i;
	unsigned char *vector;
	
	os_printf("Reading initial config\n");

	if (spi_flash_read(CONFIG_ADDRESS, (uint32*) &confRead, sizeof(struct config)) != SPI_FLASH_RESULT_OK) {
		os_printf("Error reading stored data from address: %x.\n", CONFIG_ADDRESS);
		_default_data();
	}

	vector = (unsigned char *) os_malloc(sizeof(confRead));
	os_memcpy(vector, &confRead, sizeof(confRead));

	for (i = 2; i < sizeof(confRead); i++) {
		sum1 = (sum1 + vector[i])%255;
		sum2 = (sum2 + sum1)%255;
	}

	os_free(vector);	

	os_printf("Checksum: %d\n",((sum2<<8)|sum1));
/*	
	if (((sum2<<8)|sum1) != confRead.checksum) {
		os_printf("Checksum error.\n");
		_default_data();
	}
*/
}

// Write config
int _write(struct config conf) {
	ETS_UART_INTR_DISABLE();

	if (spi_flash_erase_sector(CONFIG_SECTOR) != SPI_FLASH_RESULT_OK) {
		os_printf("Error erasing sector CONFIG_SECTOR.\n");
		ETS_UART_INTR_ENABLE();
		return 1;
	}

	if (spi_flash_write(CONFIG_ADDRESS, (uint32*) &conf, sizeof(struct config)) != SPI_FLASH_RESULT_OK) {
		os_printf("Error writing data to sector: %d\n", CONFIG_SECTOR);
		ETS_UART_INTR_ENABLE();
		return 1;
	}

	os_printf("Data saved correctly.\n");
	ETS_UART_INTR_ENABLE();
	return 0;
}

// Set default data 
void _default_data() {
	confRead.hum = DEFAULT_HUM;
	confRead.temp = DEFAULT_TEMP;
	confRead.time = DEFAULT_TIME;
	confRead.off = DEFAULT_OFF;

	if (config_save(confRead)) os_printf ("Error saving default data\n");
}
