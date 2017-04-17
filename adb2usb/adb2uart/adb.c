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


#define ADB_H() DDRB&=~(1<<ADBPIN)
#define ADB_L() DDRB|=(1<<ADBPIN)
#define ADBVAL (PINB&(1<<ADBPIN))

static void adbSendOne(void) {
	ADB_L();
	_delay_us(35);
	ADB_H();
	_delay_us(65);
}

static void adbSendZero(void) {
	ADB_L();
	_delay_us(65);
	ADB_H();
	_delay_us(35);
}

static void adbAttnSync(void) {
	ADB_L();
	_delay_us(800);
	ADB_H();
	_delay_us(70);
}

static void adbSendByte(unsigned char byte) {
	unsigned char p;
	for (p=0; p<8; p++) {
		if (byte&128) adbSendOne(); else adbSendZero();
		byte<<=1;
	}
}

//Includes stop byte
static void adbSendTlt(void) {
	adbSendZero();
	ADB_H();
//	_delay_us(165); //165 is according to spec
	_delay_us(100);
}

static char adbWaitStart(void) {
	int x=200;
	//Wait till line goes low.
	_delay_us(1);
	while (ADBVAL && x>0) {
		x--;
		_delay_us(1);
	}
	if (x==0) return 0;

	//Delay for the low time of the start bit
	_delay_us(40);
	//Should be high again...
	if (!ADBVAL) return 0;
	//Wait till the line goes low again for the 1st databit.
	while(ADBVAL && x>0) {
		x--;
		_delay_us(1);
	}
	if (x==0) return 0;
	//Ok, all good.
	return 1;
}

static char adbGetBit(void) {
	char res=0;
	int x;
//	DDRD|=(1<<3);
//	PIND=(1<<3);
	_delay_us(50);
//	PIND=(1<<3);
	if (ADBVAL) res=1;
	x=80;
	while(!ADBVAL && x>0) {
		_delay_us(1);
		x--;
	}
	while(ADBVAL && x>0) {
		_delay_us(1);
		x--;
	}
	if (x==0) return -1;
	return res;
}

static unsigned char adbGetByte(void) {
	unsigned char ret, b, x;
	for (x=0; x<8; x++) {
		ret<<=1;
		b=adbGetBit();
		if (b==-1) return 0xff;
		if (b==1) ret|=1;
	}
	return ret;
}


int adbCommand(unsigned char cmd, unsigned char *data, int size) {
	int x;
	adbAttnSync();
	adbSendByte(cmd);
	adbSendTlt();
	if (cmd&C_TALK) {
		//Device writes data
		if (!adbWaitStart()) return -1;
		for (x=0; x<size; x++) data[x]=adbGetByte();
	} else {
		//We write data.
		_delay_us(200);
		adbSendOne();
		for (x=0; x<size; x++) adbSendByte(data[x]);
		adbSendOne();
	}
	return size;
}

void adbReset(void) {
	PORTD&=~(1<<ADBPIN);

	//Reset bus
	ADB_L();
	_delay_ms(3);
	ADB_H();
	_delay_ms(2);
}
