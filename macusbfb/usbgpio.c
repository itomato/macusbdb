/*
Firmware for a Macintosh display adapter connected over USB
Copyright (C) 2010 Jeroen Domburg

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "gpio.h"
#include "usbgpio.h"

//Handler for the USB-exposed GPIO pins

struct GpioPins_t {
	int port;
	int bit;
};

const struct GpioPins_t gpioPins[]={
	{0, 11},	//Fan enable, can be pwmmed too
	{2, 2},		//Monitor enable
	{2, 10},	//Floppy eject
	{3, 2},		//HD led
}


void gpioSet(int gpio, int val) {
	if (gpio>4) return;
	if (val) {
		GPIOSET(gpioPins[gpio].port, gpioPins[gpio].bit);
	} else {
		GPIOCLEAR(gpioPins[gpio].port, gpioPins[gpio].bit);
	}
}

int gpioGet(int gpio) {
	//Not implemented (yet)
}

