/*
ADB to USB-converter, for a non-extended keyboard and a mouse
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
#define F_CPU 12000000
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "adb.h"

#include "scanadb2usb.h"


#define UDR0 UDR
#define UCSR0A UCSRA
#define UCSR0B UCSRB
#define UCSR0C UCSRC
#define UBRR0L UBRRL
#define UBRR0H UBRRH


void sendchar(char c) {
	while(!(UCSR0A&(1<<5))) ;
	UDR0=c;
}

//Main routine
int main(void) {
	unsigned char data[8];
	unsigned char keys[4]={0,0,0,0};
	unsigned char modFlags=0;
	int ret;
	int x;
//	stdoutInit(39); //1228800MHz -> 19200 baud
    UCSR0B=0x18;
    UBRR0L=39;
    UBRR0H=0;
//	sei();
//	printf_P(PSTR("PWRON\n"));
	PORTD&=~(1<<ADBPIN);

	adbReset();

	while(1) {
		ret=adbCommand((3<<4)|C_TALK|REG_0, data, 2);
		if (ret>0) {
			char n;
			sendchar('M');
			sendchar((data[0]&0x80)?0:1);
			n=data[1]&0x7f; if (data[1]&0x40) n+=128;
			sendchar(n);
			n=data[0]&0x7f; if (data[0]&0x40) n+=128;
			sendchar(n);
//			printf("Mouse: %i %i, but=%i\n", data[0]&0x7f, data[1]&0x7f, (data[0]&0x80)?0:1);
//			printf("Mouse: %x %x %x %x\n", data[0], data[1], data[2], data[3]);
		}
		ret=adbCommand((2<<4)|C_TALK|REG_0, data, 2);
		if (ret>0) {
			int x=0;
			char mask=0;
			unsigned char usbkey;
			unsigned int key=data[0]&0x7F;
			char press=data[0]&0x80;
			if (key==0x7F) key=(data[1]&0x7f)+0x80;
//			printf("Keyboard: key %3x %s.\n", key, press?"released":"pressed");
			while(pgm_read_byte(&scanadb2usb[x].usb)!=0) {
				if (pgm_read_byte(&scanadb2usb[x].adb)==key) usbkey=pgm_read_byte(&scanadb2usb[x].usb);
				x++;
			}
			x=0;

			//Check meta keys
			mask=0;
			if (key==0x36) mask=0x01;
			if (key==0x38) mask=0x02;
			if (key==0x37) mask=0x04;
			if (key==0x7d) mask=0x10;
			if (key==0x7b) mask=0x20;

			if (!mask) {
				if (!press) {
					while (x!=4 && keys[x]!=0) x++;
					if (x!=4) keys[x]=usbkey;
				} else {
					for (x=0; x<4; x++) {
						if (keys[x]==usbkey) keys[x]=0;
					}
				}
			} else {
				if (!press) {
					modFlags|=mask;
				} else {
					modFlags&=~mask;
				}
			}
			sendchar('K');
			sendchar(modFlags);
			sendchar(keys[0]);
			sendchar(keys[1]);
			sendchar(keys[2]);
			sendchar(keys[3]);
		}
		_delay_ms(9);
	}
}
