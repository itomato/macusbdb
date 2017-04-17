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
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
//#include "usbdrv/usbdrv.c"
#include "oddebug.h"        /* This is also an example for using debug macros */


/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

char usbHidReportDescriptor[] PROGMEM = {
	/* partial keyboard */
	0x05, 0x01,	/* Usage Page (Generic Desktop), */
	0x09, 0x06,	/* Usage (Keyboard), */
	0xA1, 0x01,	/* Collection (Application), */
	0x85, 0x01,		/* Report Id (1) */
	0x05, 0x07,            //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,            //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,            //   USAGE_MAXIMUM (Keyboard Right GUI)

	0x15, 0x00,            //   LOGICAL_MINIMUM (0)
	0x25, 0x01,            //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,            //   REPORT_SIZE (1)
	0x95, 0x08,            //   REPORT_COUNT (8)
	0x81, 0x02,            //   INPUT (Data,Var,Abs)

	0x05, 0x07,		/* Usage Page (Key Codes), */
	0x95, 0x04,		/* Report Count (4), */
	0x75, 0x08,		/* Report Size (8), */
	0x15, 0x00,		/* Logical Minimum (0), */
	0x25, 0x75,		/* Logical Maximum(117), */
	0x19, 0x00,		/* Usage Minimum (0), */
	0x29, 0x75,		/* Usage Maximum (117), */
	0x81, 0x00,		/* Input (Data, Array),               ;Key arrays (4 bytes) */
	0xC0,		/* End Collection */

	/* mouse */
	0x05, 0x01,	/* Usage Page (Generic Desktop), */
	0x09, 0x02,	/* Usage (Mouse), */
	0xA1, 0x01,	/* Collection (Application), */
	0x09, 0x01,	/*   Usage (Pointer), */
	0xA1, 0x00,	/*   Collection (Physical), */
	0x05, 0x09,		/* Usage Page (Buttons), */
	0x19, 0x01,		/* Usage Minimum (01), */
	0x29, 0x03,		/* Usage Maximun (03), */
	0x15, 0x00,		/* Logical Minimum (0), */
	0x25, 0x01,		/* Logical Maximum (1), */
	0x85, 0x02,		/* Report Id (2) */
	0x95, 0x03,		/* Report Count (3), */
	0x75, 0x01,		/* Report Size (1), */
	0x81, 0x02,		/* Input (Data, Variable, Absolute), ;3 button bits */
	0x95, 0x01,		/* Report Count (1), */
	0x75, 0x05,		/* Report Size (5), */
	0x81, 0x01,		/* Input (Constant),                 ;5 bit padding */
	0x05, 0x01,		/* Usage Page (Generic Desktop), */
	0x09, 0x30,		/* Usage (X), */
	0x09, 0x31,		/* Usage (Y), */
	0x15, 0x81,		/* Logical Minimum (-127), */
	0x25, 0x7F,		/* Logical Maximum (127), */
	0x75, 0x08,		/* Report Size (8), */
	0x95, 0x02,		/* Report Count (2), */
	0x81, 0x06,		/* Input (Data, Variable, Relative), ;2 position bytes (X & Y) */
	0xC0,		/*   End Collection, */
	0xC0,		/* End Collection */
};

typedef struct {
	char	id;
	char	meta;
	char	b[4];
}keybReport_t;

typedef struct{
	char    id;
    uchar   buttonMask;
    char    dx;
    char    dy;
}mouseReport_t;

static keybReport_t keybReportBuffer={1, 0, {0, 0, 0, 0}};
static mouseReport_t mouseReportBuffer={2, 0, 0, 0};
static uchar    idleRate;   /* repeat rate for keyboards, never used for mice */
static char doSend;

#define SEND_MOUSE 1
#define SEND_KEYB 2


//Parses a byte from the ADB receiving microcontroller.
//Includes a state machne to convert the individual bytes into a
//HID report we can send.
static void parseByte(unsigned char b) {
	static unsigned char buff[8];
	static int toGet=0; //Bytes left in this hid report
	static int p=0; //Buffer index
	if (toGet==0) {
		if (b=='M') {
			toGet=4;
		} else if (b=='K') {
			toGet=6;
		}
	}
	if (toGet>0) {
		buff[p++]=b;
		toGet--;
		if (toGet==0) { //Got all bytes we need?
			if (buff[0]=='M') {
				mouseReportBuffer.buttonMask=buff[1];
				mouseReportBuffer.dx+=buff[2];
				mouseReportBuffer.dy+=buff[3];
				doSend|=SEND_MOUSE;
			} else if (buff[0]=='K') {
				keybReportBuffer.meta=buff[1];
				keybReportBuffer.b[0]=buff[2];
				keybReportBuffer.b[1]=buff[3];
				keybReportBuffer.b[2]=buff[4];
				keybReportBuffer.b[3]=buff[5];
				doSend|=SEND_KEYB;
			}
		p=0;
		}
	}
}


usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t    *rq = (void *)data;
    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            if (rq->wValue.bytes[1]==1) {
	            usbMsgPtr = (void *)&mouseReportBuffer;
    	        return sizeof(mouseReportBuffer);
            } else if (rq->wValue.bytes[1]==2) {
	            usbMsgPtr = (void *)&mouseReportBuffer;
    	        return sizeof(mouseReportBuffer);
			}
        } else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        } else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void) {
	uchar   i;
    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
	DDRD=0;
	UCSRB=0x18;
	UBRRL=39;
	UBRRH=0;
    odDebugInit();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             // fake USB disconnect for > 250 ms
        wdt_reset();
        _delay_ms(1);
    }

    usbDeviceConnect();
    sei();
    for(;;){                /* main event loop */
        wdt_reset();
        usbPoll();
        if(usbInterruptIsReady()){
			if (doSend&SEND_MOUSE) {
	            /* called after every poll of the interrupt endpoint */
    	        usbSetInterrupt((void *)&mouseReportBuffer, sizeof(mouseReportBuffer));
				mouseReportBuffer.dx=0;
				mouseReportBuffer.dy=0;
				mouseReportBuffer.buttonMask=0;
				doSend&=~SEND_MOUSE;
			} else if (doSend&SEND_KEYB) {
    	        usbSetInterrupt((void *)&keybReportBuffer, sizeof(keybReportBuffer));
				keybReportBuffer.b[0]=0;
				doSend&=~SEND_KEYB;
			}
        }
		if (UCSRA&(1<<7)) parseByte(UDR);
    }
}

/* ------------------------------------------------------------------------- */
