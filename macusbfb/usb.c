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
//Implement a simple USB interface thingy which can receive bulk packets.
//No interrupts are used because of timing issues in the main code.

#include "lpc134x.h"
#include <stdio.h>

#define NOP() __asm volatile ("NOP")
#define PACKED __attribute__((__packed__)) 

//Comment out to get debugging stuff on the monitor.
#define printf(bla, ...) {}

extern void gpioSet(int gpio, int val);

typedef struct {
	unsigned char bmRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
} PACKED setupPacketTp;

//Descriptor ref:
//http://www.usbmadesimple.co.uk/ums_4.htm

char myDevDescriptor[]={
	18, 		//length (18?)
	1, 			//descriptor type
	0x02,0x00,	//usb ver
	0xff,		//class
	0,			//subclass
	0,			//protocol
	64,			//max packet size
	0x34,0x12,	//vendor id
	0xAA,0x55,	//product id
	0x00,0x01,	//device release
	0x00,		//manuf str index
	0x00,		//prod str index
	0x00,		//serial str index
	0x01		//no of possible configs
};

char myConfDescriptor[]={
	//main config descriptor
	9,			//length of this descr
	02,			//type (config descr)
	32,0,		//total length
	01,			//no of interfaces
	01,			//select config no 1 to use this
	00,			//string descr of this config
	0x80,		//attributes
	100,		//max current drawn in 2ma steps
	//iface1 condfig descriptor
	9,			//len
	4,			//type (iface descr)
	0,			//iface number this descr describes
	0,			//alternate setting for this iface
	2,			//number of endpoints
	0xff,		//iface class
	0,			//Iface subclass
	0,			//Iface protocol
	0,			//String descr of this iface
	//Endpoint descriptor
	7,			//len
	5,			//type (endpoint descr)
	0x3,		//address. IN iface = +0x80
	0x2,		//attributes
	32,0,		//max packet size
	10,			//Interval to poll for data, in ms
	//Endpoint descriptor
	7,			//len
	5,			//type (endpoint descr)
	0x83,		//address. IN iface = +0x80
	0x2,		//attributes
	32,0,		//max packet size
	10,			//Interval to poll for data, in ms
};

static int myAddr=0;

static void dumpHex(unsigned char *buf, int len) {
	int x;
	if (len==0) return;
	for (x=0; x<len; x++) {
		if ((x&15)==8) printf(" ");
		if ((x&15)==0) {
			if (x!=0) printf("\n");
			printf("%04x: ",x);
		}
		printf("%02x ", (int)buf[x]);
	}
	printf("\n");
}


static inline void wrCmd(int cmd) {
	USB_DEVINTCLR = (1<<10);
	USB_CMDCODE=0x0500|(cmd<<16);
	while ((USB_DEVINTST & ((1<<10)|(1<<9))) == 0);
}

static inline void wrData(int data) {
	USB_DEVINTCLR = (1<<10);
	USB_CMDCODE=0x0100|(data<<16);
	while ((USB_DEVINTST & ((1<<10)|(1<<9))) == 0);
}

static inline int rdCmd(int cmd) {
	wrCmd(cmd);
	USB_DEVINTCLR=(1<<10)|(1<<11);
	USB_CMDCODE=0x200|(cmd<<16);
	while ((USB_DEVINTST&((1<<11)|(1<<9))) == 0);
	return (USB_CMDDATA);
}


void usbInit() {
	IOCON_PIO0_3 &= ~IOCON_PIO0_3_FUNC_MASK;
	IOCON_PIO0_3 |= IOCON_PIO0_3_FUNC_USB_VBUS;         // VBus
	IOCON_PIO0_6 &= ~IOCON_PIO0_6_FUNC_MASK;
	IOCON_PIO0_6 |= IOCON_PIO0_6_FUNC_USB_CONNECT;      // Soft Connect


	/* Enable Timer32_1, IOCON, and USB blocks */
	SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B1 | SCB_SYSAHBCLKCTRL_IOCON | SCB_SYSAHBCLKCTRL_USB_REG);

	// Setup USB clock
	SCB_PDRUNCFG &= ~(SCB_PDSLEEPCFG_USBPAD_PD);        // Power-up USB PHY
	SCB_PDRUNCFG &= ~(SCB_PDSLEEPCFG_USBPLL_PD);        // Power-up USB PLL

	SCB_USBPLLCLKSEL = SCB_USBPLLCLKSEL_SOURCE_MAINOSC; // Select PLL Input
	SCB_USBPLLCLKUEN = SCB_USBPLLCLKUEN_UPDATE;         // Update Clock Source
	SCB_USBPLLCLKUEN = SCB_USBPLLCLKUEN_DISABLE;        // Toggle Update Register
	SCB_USBPLLCLKUEN = SCB_USBPLLCLKUEN_UPDATE;

	// Wait until the USB clock is updated
	while (!(SCB_USBPLLCLKUEN & SCB_USBPLLCLKUEN_UPDATE));

	// Set USB clock to 48MHz (12MHz x 4)
	SCB_USBPLLCTRL = (SCB_USBPLLCTRL_MULT_4);
	while (!(SCB_USBPLLSTAT & SCB_USBPLLSTAT_LOCK));    // Wait Until PLL Locked
	SCB_USBCLKSEL = SCB_USBCLKSEL_SOURCE_USBPLLOUT;

	//Set address 0, enable usb
	wrCmd(0xD0);
	wrData(0x80);
	wrCmd(0xD0);
	wrData(0x80);

}

void usbConnect() {
	//Connect
	wrCmd(0xFE);
	wrData(0x1);
}

void writeToEp(int ep, unsigned int *data, int len) {
	int x;


	USB_CTRL=((ep&0xF)<<2)|(1<<1);
	NOP(); NOP(); NOP();
	USB_TXPLEN=len;

	for (x=0; x<(len); x+=4) {
		USB_TXDATA=data[x/4];
	}
	USB_CTRL=0;

	wrCmd((ep<<1)+1);
	wrCmd(0xFA);

	//Dunno why, detection seems to hinge on a delay here.
	for (x=0; x<10000; x++) NOP();
	printf("Written %i bytes to ep %i.\n", len, ep);
//	if (len>0) dumpHex((unsigned char* )data, len);
}

int readFromEp(int ep, unsigned int *data, int maxlen) {
	int x, dummy;
	USB_CTRL=((ep&0xF)<<2)|(1<<0);
	NOP(); NOP(); NOP();
	int plen;
	do {
		plen=USB_RXPLEN;
	} while ((plen&0x400)==0);
	plen&=0x3ff;

	if (plen!=maxlen) printf("Ep %x: recv %i bufflen=%i!\n", ep, plen, maxlen);
	for (x=0; x<(plen); x+=4) {
		if (x<maxlen) {
			data[x/4]=USB_RXDATA;
		} else {
			dummy=USB_RXDATA;
		}
	}

	USB_CTRL=0;

	wrCmd(ep<<1);
	wrCmd(0xF2);

	return plen>maxlen?maxlen:plen;
}

inline short swap16(unsigned short word) {
	return ((word&0xff)<<8)|(word>>8);
}

static inline void handleSetupPacket(setupPacketTp *p) {
//A	printf("Setup packet, type=%i, req=%i, val=%i, idx=%i\n", p->bmRequestType, p->bRequest, p->wValue, p->wIndex);
//	dumpHex((unsigned char* )p, 8);
	if (p->bmRequestType==0x80 && p->bRequest==6) {
		//Get descriptor.
		int index=swap16(p->wIndex);
		int len=p->wLength;
		if (swap16(p->wValue)==1) {
			//Dev descriptor. Write to ep.
			writeToEp(0, (unsigned int *)myDevDescriptor, myDevDescriptor[0]);
 			printf("Dev desc written.\n");
		} else if (swap16(p->wValue)==2) {
			int resplen=myConfDescriptor[2]+(myConfDescriptor[3]<<8);
			if (resplen>len) resplen=len;
			writeToEp(0, (unsigned int *)myConfDescriptor, len);
			printf("Conf desc written.\n");
		} else {
			printf("EEK! Unhandled descriptor request!\n");
		}
	} else if (p->bmRequestType==00 && p->bRequest==5) {
		//Set address
		//First send Ack with our old addresss 0
		writeToEp(0, NULL, 0);
		//Then set new address.
		myAddr=(p->wValue&0x7F);

		wrCmd(0xD0);
		wrData(myAddr|0x80);
		wrCmd(0xD0);
		wrData(myAddr|0x80);
		printf("Set addr to %i.\n",myAddr);
	} else if (p->bmRequestType==00 && p->bRequest==9) {
		//Set configuration.
		//Just allow, there's only one of them anyway.
		writeToEp(0, NULL, 0);
		//Ok, other stuff can come in now.
		wrCmd(0xD8);
		wrData(1);
		printf("Enabled other endpoints!\n");
	} else if (p->bmRequestType==0x40 && p->bRequest==1) { //vendor specific, gpio control, host->dev
		//Modify output wIndex to wValue
		gpioSet(p->wIndex, p->wValue);
	} else {
		printf("EEK! Unhandled setup packet!\n");
	}
}

extern char usbData[64];
extern int usbDataLen;


void usbHandle() {
	//Handle all the USB stuff by looking at any interrupts that would have been triggered.
	int ints=USB_DEVINTST;
	USB_DEVINTCLR=ints;
//	if ((ints&0x1fe)!=0) printf("Int status %x\n", ints);
	if (ints&(1<<7)) {
		int epstatus=rdCmd(0x46);
		printf("Ep3 int, status=%x\n", epstatus);
		usbDataLen=readFromEp(3, usbData, 64);
		return;
	}
	if (ints&(1<<8)) {
		int epstatus=rdCmd(0x47);
		printf("Ep3.1 int, status=%x\n", epstatus);
	}

	if (ints&(1<<1)) {
		//Endpoint0 int
		int epstatus=rdCmd(0x40);
//		printf("Ep0 int, status=%x\n", epstatus);
		if (epstatus&(1<<2)) {
			//Setup-packet.
			unsigned char setupPacket[8];
			readFromEp(0, (unsigned int *)setupPacket, 8);
			handleSetupPacket((setupPacketTp*) setupPacket);
		}
	}
	if (ints&(1<<9)) {
		//Device status change
		int status=rdCmd(0xfe);
//		printf("Dev status is now %x\n",status);
		if (!(status&1)) {
			//Reconnect, damn you!
/*
			wrCmd(0xFE);
			wrData(0x1);
			myAddr=0;
*/
		}
	}
}
