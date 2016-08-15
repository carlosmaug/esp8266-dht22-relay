/****************************************************************************
 * Copyright (C) by Jeroen Domburg <jeroen@spritesmods.com>                 *
 *                                                                          *
 * As long as you retain this notice you can do whatever you want with this *
 * stuff. If we meet some day, and you think this stuff is worth it, you    *
 * can buy me a beer in return.                                             *
 ****************************************************************************/
/******************************************************************************
 * pollDHTCb                                                                  *
 *                                                                            *
 * Originally from:                                                           * 
 * http://harizanov.com/2014/11/esp8266-powered-web-server-led-control-dht22-temperaturehumidity-sensor-reading/
 * Adapted from: https://github.com/adafruit/Adafruit_Python_DHT/blob/master/source/Raspberry_Pi/pi_dht_read.c
 * LICENSE:                                                                   *
 * Copyright (c) 2014 Adafruit Industries                                     *
 * Author: Tony DiCola                                                        *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included    *
 * in all copies or substantial portions of the Software.                     *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS    *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,*
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE*
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 ******************************************************************************/

/**
 * @file dht.c
 * @author Jeroen Domburg <jeroen@spritesmods.com>
 * @contributors Carlos Martin Ugalde and Ignacio Ripoll García
 * @brief File containing all functions needed to access DTH sensor
 * interface
 *
 * 
 * This file contains DHT driver and other funcionts related. 
 */

#include <esp8266.h>

#include <dht.h>

#define MAXTIMINGS 10000
#define DHT_MAXCOUNT 32000
#define BREAKTIME 32

//Debug 1 = on
#define DEBUG 0
#define DHT_PIN 0 

enum EDhtType SENSOR;

void ets_intr_lock(void);
void ets_intr_unlock(void);

static struct DhtReading reading = {
	.success = 0
};

/*
 * @brief Convert DHT humidity outpunt into % units
 */

static inline float _scale_humidity(int *data) {
	if (SENSOR == SENSOR_DHT11) {
		return data[0];
	} else {
		float humidity = data[0] * 256 + data[1];
		return humidity /= 10;
	}
}

/*
 * @brief Convert DHT temterature outpunt into celsious degrees 
 */

static inline float _scale_temperature(int *data) {
	if (SENSOR == SENSOR_DHT11) {
		return data[2];
	} else {
		float temperature = data[2] & 0x7f;
		temperature *= 256;
		temperature += data[3];
		temperature /= 10;
		if (data[2] & 0x80)
			temperature *= -1;
		return temperature;
	}
}

/*
 * @brief Wait sleep milliseconds
 *
 * This function is used to talk to DHT sensor 
 */

static inline void _delay_ms(int sleep) { 
		os_delay_us(1000 * sleep); 
}

/*
 * @brief Read DTH sensot 
 *
 * This function is used to read DHT sensor
 * See http://www.electrodragon.com/w/DHT22_Digital_Humidity_and_Temperature_Sensor_%28AM2302%29 
 */

static	void ICACHE_FLASH_ATTR _poll_dht_cb(void *arg){
	int counter = 0;
	int laststate = 1;
	int i = 0;
	int bits_in = 0;
	int data[100];

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	//disable interrupts, start of critical section
	ets_intr_lock();

	// Wake up device, 250ms of high
	GPIO_OUTPUT_SET(DHT_PIN, 1);
	_delay_ms(20);

	// Hold low for 20ms
	GPIO_OUTPUT_SET(DHT_PIN, 0);
	_delay_ms(20);

	// High for 40ms
	GPIO_DIS_OUTPUT(DHT_PIN);
	os_delay_us(40);

	// wait for pin to drop?
	while (GPIO_INPUT_GET(DHT_PIN) == 1 && i < DHT_MAXCOUNT) {
		if (i >= DHT_MAXCOUNT) {
			os_printf("ERROR: Timeout reading DHT\n");
			reading.success = 0;
			return;
		}
		i++;
	}

#if DEBUG
	os_printf("Reading DHT\n");
#endif

	// read data!
	for (i = 0; i < MAXTIMINGS; i++) {
		// Count high time (in approx us)
		counter = 0;
		while (GPIO_INPUT_GET(DHT_PIN) == laststate) {
			counter++;
			os_delay_us(1);
			if (counter == 1000)
				break;
		}

		laststate = GPIO_INPUT_GET(DHT_PIN);

		if (counter == 1000)
			break;

		// store data after 3 reads
		if ((i > 3) && (i % 2 == 0)) {
			// shove each bit into the storage bytes
			data[bits_in / 8] <<= 1;

			if (counter > BREAKTIME) {
				data[bits_in / 8] |= 1;
			}

			bits_in++;
		}
	}

	//Re-enable interrupts, end of critical section
	ets_intr_unlock();

	if (bits_in < 40) {
		os_printf("ERROR: Reading DHT, got too few bits: %d should be at least 40\n", bits_in);
		reading.success = 0;
		return;
	}
	
	int checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;

#if DEBUG	
	os_printf("DHT: %02x %02x %02x %02x [%02x] CS: %02x\n", data[0], data[1],data[2],data[3],data[4],checksum);
#endif

	if (data[4] != checksum) {
		os_printf("ERROR: Reading DHT, Checksum was incorrect after %d bits. Expected %d but got %d\n",
							bits_in, data[4], checksum);
		reading.success = 0;
	} else {
		reading.temperature = _scale_temperature(data);
		reading.humidity = _scale_humidity(data);
		os_printf("Temp = %d *C, Hum = %d %%\n", (int)(reading.temperature * 100),
						 (int)(reading.humidity * 100));
		reading.success = 1;
        }
	return;
}

/*
 * @brief Returns DHT data
 *
 * Return last data readed from DHT or force an new reading.
 */

struct DhtReading *ICACHE_FLASH_ATTR dht_read(int force) {
	if (force) {
		_poll_dht_cb(NULL);
	} 

	return &reading;
}

/*
 * @brief Init DHT
 *
 */

void dht_init(enum EDhtType DhtType, uint32_t polltime) {
        SENSOR = DhtType;
	// Set GPIO to output mode for DHT22
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	
	os_printf("Starting readings of DHT type %d, poll interval of %d\n", SENSOR, (int)polltime);
	
	static ETSTimer dhtTimer;
	os_timer_setfn(&dhtTimer, _poll_dht_cb, NULL);
	os_timer_arm(&dhtTimer, polltime, 1);
}
