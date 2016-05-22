# ESP8266 model 1  DHT22 or DHT11 realy control

Turns on/off a relay based on reading of temperature and humidity of a DHT22 or DHT11 sensor. Optionally you can turn on/off relay manually using the web interface.

Web interface lets you connect to your wifi, configure defaut values, see DHT readings, turn on/off the relay, and other things.

In this version you will find:
- New Web interface, now responsive.
- New configuration web page added.
- New Relay control page.
- Configuration is kept after reset.
- Fallback to AP mode in case of loosing access to AP.
- Tons of code commenting, cleanup and beautifying.
- Tons of code and circuit testing.
- Added circuit schema.
- All configuration is now placed in a single file.
- Added deug mode to reduce console vervosity.
- Compatibility with esp iot sdk v1.2.0 (it does not compile against newer versions)

Seccurity issues
- MANY.... 
- Wifi password is very easy to retrieve from the web interface.
- No support for ssl nor tls, or any other form of encription.
- No password support.
- It is still pending the use of snprintf.
- ETC..

# Configuration

There are a few parameters that can be changed:
 * include/config.h: Temperature and humidity boundaries to turn on relay, number of  minutes to keey relay on since it was manually turned on, sensor type (DHT11 or DHT22) and poll rate
 
# Building

Make sure the IoT SDK and toolchain are set up according to the instructions on the [ESP8266 wiki](https://github.com/esp8266/esp8266-wiki/wiki/Toolchain). The makefile in this project relies on some environment variables that need to be set. It should be enough to add these to your `.profile`:

	PATH="/opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin/:$PATH"
	export SDK_BASE="/opt/Espressif/ESP8266_SDK"
	export XTENSA_TOOLS_ROOT="/opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin/"
	export ESPTOOL="/opt/Espressif/esptool-py/esptool.py"

From the root of this repository, run `make`. This will build the two firmware images. To update the webserver's HTML files, run `make webpages.espfs`.

# Screenshots

## Web interface

![index](https://raw.githubusercontent.com/carlosmaug/esp8266-dht22-relay/master/screenshots/index.png)
![dht](https://raw.githubusercontent.com/carlosmaug/esp8266-dht22-relay/master/screenshots/dht.png)
![Realy settings](https://raw.githubusercontent.com/carlosmaug/esp8266-dht22-relay/master/screenshots/relay_config.png)
![WIFI config](https://raw.githubusercontent.com/carlosmaug/esp8266-dht22-relay/master/screenshots/wifi_config.png)

# Hardware

We have included all information and schemas inside the eagle directory. You can edit them using eagle software.

![Schema](https://raw.githubusercontent.com/carlosmaug/esp8266-dht22-relay/master/screenshots/esquema.png)

# Background

This application is based on [Mathew Hall](https://github.com/mathew-hall/esp8266-dht) software, which in turn is based on others, like [Martin's DHT22 webserver](http://harizanov.com/2014/11/esp8266-powered-web-server-led-control-dht22-temperaturehumidity-sensor-reading/) implementation, which uses [sprite_tm's ESP8266 httpd](http://www.esp8266.com/viewtopic.php?f=6&t=376).


# Licenses

Please refer to each file to see the licences.
