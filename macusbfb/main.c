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
#include "sysinit.h"
#include "lpc134x.h"
#include <stdio.h>
#include "usb.h"
#include "gpio.h"

//Mac display = 512x342
//Pixel clock = 15.6672MHz

//Refresh freq = 60.15
//Scan rate: 22KHz (?)
//Vbl = 28 scan lines
//Hbl = 192 pixels
// Total: 714x370 'pixels'
//1 line = 64 bytes aan data

//Ok, the HBL is really funky and actually a HDrive...
//More info: http://members.optusnet.com.au/eviltim/macmp3/macmp3.htm


/*
Timings:
Per line: 512 pixels, 202 non-visible 'pixels'
SPI FIFO buffer is 128 pixels -> 384 pixels in which we have to keep pushing the buffer, 330 in which we don't.

*/


#define NOP() __asm volatile ("NOP")


#define LEDPORT 2
#define LEDBIT 6
#define HSYNCPORT 0
#define HSYNCBIT 10 //=CT16B0_MAT2
#define VSYNCPORT 0
#define VSYNCBIT 7
#define VIDEOPORT 0
#define VIDEOBIT 9

#define DRAMDPORT 1
#define DRAMAPORT 1
//DRAM D pins: P1_8 - P1_11
//DRAM A pins: P1_0 - P1-7
#define DRAMDMASK 0x0F00
#define DRAMDSHIFT 8
#define DRAMAMASK 0x00FF
#define DRAMASHIFT 0
#define DRAMWPORT 3
#define DRAMWBIT 1
#define DRAMRASPORT 3
#define DRAMRASBIT 0
#define DRAMCASPORT 3
#define DRAMCASBIT 3
#define DRAMGPORT 2
#define DRAMGBIT 11

#define GPIOFANPORT 2
#define GPIOFANBIT 4
#define GPIOMONPORT 2
#define GPIOMONBIT 2
#define GPIOEJECTPORT 2
#define GPIOEJECTBIT 10
#define GPIOHDLEDPORT 3
#define GPIOHDLEDBIT 2

#define SENDLINE_BUFFER 1
#define SENDLINE_FREETIME 2

volatile int linePos;
volatile int wordPos;
volatile int dramRow=0;
volatile int doSendLine;
char usbData[64];
int usbDataLen;
int usbDataProcessed;

struct GpioPins_t {
	int port;
	int bit;
};

const struct GpioPins_t gpioPins[]={
	{GPIOFANPORT, GPIOFANBIT},		//Fan enable
	{GPIOMONPORT, GPIOMONBIT},		//Monitor enable
	{GPIOEJECTPORT, GPIOEJECTBIT},	//Floppy eject
	{GPIOHDLEDPORT, GPIOHDLEDBIT},	//HD led
};


void gpioSet(int gpio, int val) {
	if (gpio>=4) return;
	if (val) {
		GPIOSET(gpioPins[gpio].port, gpioPins[gpio].bit);
	} else {
		GPIOCLEAR(gpioPins[gpio].port, gpioPins[gpio].bit);
	}
}


//Timing note: One NOP is 13ns.

//Takes a while to set up the line, so this should be called a bit before the real data has to be sent.
void sendLine(int line) {
	int dramCol;

	GPIOCLEAR(LEDPORT, LEDBIT);

	GPIOMOD(DRAMAPORT, DRAMAMASK, line<<DRAMASHIFT);
	GPIOCLEAR(DRAMRASPORT, DRAMRASBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOCLEAR(DRAMGPORT, DRAMGBIT);
	GPIO_GPIO1DIR&=~DRAMDMASK;


#define GETSENDNIBBLE() \
		GPIOMOD(DRAMAPORT, DRAMAMASK, (dramCol++)<<DRAMASHIFT); \
		GPIOCLEAR(DRAMCASPORT, DRAMCASBIT); \
		NOP(); NOP(); NOP(); NOP(); \
		SSP_SSP0DR=((GPIO_GPIO1DATA&DRAMDMASK)>>DRAMDSHIFT); \
		GPIOSET(DRAMCASPORT, DRAMCASBIT);

	int from=0; int to=128;
	if (line>=256) {
		from=128;
		to=256;
	}
	for (dramCol=from; dramCol<to; ) {
		while(!(SSP_SSP0RIS&(1<<3))) ;
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
		GETSENDNIBBLE();
	}
	SSP_SSP0DR=0xf;
	GPIOSET(DRAMRASPORT, DRAMRASBIT);
	GPIOSET(LEDPORT, LEDBIT);
}

//Process bytes coming in over the usb-interface.
//First=1 on first call in a row, 2 on last.
//Returns an estimate of the time it took.
inline int processByte(unsigned char byte, int firstlast) {
	int ret=1;
	static enum {
		stSync=0,
		stAddrH,
		stAddrL,
		stLen,
		stRunningFirst,
		stRunning
	} state=0;
	static int pos;
	static int len=0;
	static int row=0;
	static int col=0;

	switch(state) {
		case stSync:
			if (byte==0xfa) state=stAddrH;
			break;
		case stAddrH:
			pos=byte<<8;
			state=stAddrL;
			break;
		case stAddrL:
			pos|=byte;
			state=stLen;
			break;
		case stLen:
			len=byte;
			if (len>32) state=stSync;
			//Pre-calc row and col.
			row=(pos>>6);
			if (pos<16384) {
				col=(pos&0x3f)<<1;
			} else {
				col=((pos&0x3f)|0x40)<<1;
			}
			state=stRunningFirst;
			break;
		case stRunning:
		case stRunningFirst:
			if (state==stRunningFirst || (firstlast&1)) {
				GPIO_GPIO1DIR=DRAMDMASK|DRAMAMASK;
				GPIOMOD(DRAMAPORT, DRAMAMASK, row<<DRAMASHIFT);
				GPIOCLEAR(DRAMRASPORT, DRAMRASBIT);
				GPIOCLEAR(DRAMWPORT, DRAMWBIT);
//				NOP(); NOP(); NOP(); NOP();
				state=stRunning;
				ret=2;
			}
			GPIOMOD(DRAMDPORT, DRAMDMASK, (byte>>4)<<DRAMDSHIFT);
			GPIOMOD(DRAMAPORT, DRAMAMASK, (col++)<<DRAMASHIFT);
			GPIOCLEAR(DRAMCASPORT, DRAMCASBIT);
			NOP(); NOP(); len--; if (len==0) state=stSync;
			GPIOSET(DRAMCASPORT, DRAMCASBIT);

			GPIOMOD(DRAMDPORT, DRAMDMASK, (byte)<<DRAMDSHIFT);
			GPIOMOD(DRAMAPORT, DRAMAMASK, (col++)<<DRAMASHIFT);
			GPIOCLEAR(DRAMCASPORT, DRAMCASBIT);
			NOP(); NOP(); NOP();
			GPIOSET(DRAMCASPORT, DRAMCASBIT);
			ret+=3;

			if (len==0 || (firstlast&2)) {
				GPIOSET(DRAMWPORT, DRAMWBIT);
				GPIOSET(DRAMRASPORT, DRAMRASBIT);
				ret++;
			}
		
			break;
		default:
			state=stSync;
			break;
	}
	return ret;
}


void dramSetByte(int x, int y, char byte) {
	int i;

	GPIO_GPIO1DIR=DRAMDMASK|DRAMAMASK;
	GPIOMOD(DRAMAPORT, DRAMAMASK, y<<DRAMASHIFT);
	GPIOCLEAR(DRAMRASPORT, DRAMRASBIT);
	NOP(); NOP(); NOP(); NOP();
	NOP(); NOP(); NOP(); NOP();

	i=byte&0xf;
	GPIOMOD(DRAMDPORT, DRAMDMASK, i<<DRAMDSHIFT);
	GPIOMOD(DRAMAPORT, DRAMAMASK, (x*2+1+(y>=256?128:0))<<DRAMASHIFT);
	GPIOCLEAR(DRAMWPORT, DRAMWBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOCLEAR(DRAMCASPORT, DRAMCASBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOSET(DRAMCASPORT, DRAMCASBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOSET(DRAMWPORT, DRAMWBIT);

	i=(byte>>4)&0xf;
	GPIOMOD(DRAMDPORT, DRAMDMASK, i<<DRAMDSHIFT);
	GPIOMOD(DRAMAPORT, DRAMAMASK, (x*2+(y>=256?128:0))<<DRAMASHIFT);
	GPIOCLEAR(DRAMWPORT, DRAMWBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOCLEAR(DRAMCASPORT, DRAMCASBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOSET(DRAMCASPORT, DRAMCASBIT);
	NOP(); NOP(); NOP(); NOP();
	GPIOSET(DRAMWPORT, DRAMWBIT);

	GPIOSET(DRAMRASPORT, DRAMRASBIT);
}


#define VIDEOLINESTART 25
#define VIDEOLINEEND VIDEOLINESTART+342
#define SYNCLINEEND 371

void TIMER16_0_IRQHandler(void) {
	//MR0: Reset. Make hblank high.
	//MR1: Start emitting pixel data
	if (TMR_TMR16B0IR&(1<<0)) {
		TMR_TMR16B0IR=1<<0;
		GPIOSET(HSYNCPORT, HSYNCBIT);
		linePos++;
		if (linePos==SYNCLINEEND) {
			linePos=0;
			GPIOCLEAR(VSYNCPORT, VSYNCBIT);
		} else if (linePos==15) {
			GPIOSET(VSYNCPORT, VSYNCBIT);
		}
	} else if (TMR_TMR16B0IR&(1<<1)) {
		TMR_TMR16B0IR=1<<1;
		if (linePos>=VIDEOLINESTART && linePos<VIDEOLINEEND) {
			doSendLine=SENDLINE_BUFFER;
			return;
		} else {
			doSendLine|=SENDLINE_FREETIME;
			if (linePos==(VIDEOLINESTART-1)) doSendLine&=~(SENDLINE_FREETIME);
		}
	}
}

int main (void){
	// Configure cpu and mandatory peripherals
	systemInit();


	//Enable clocks
	SCB_PDRUNCFG &= ~(SCB_PDRUNCFG_WDTOSC_MASK | 
                    SCB_PDRUNCFG_SYSOSC_MASK | 
                    SCB_PDRUNCFG_ADC_MASK);

	/* Enable AHB clocks */
	SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_GPIO)|(SCB_SYSAHBCLKCTRL_CT16B0)|(SCB_SYSAHBCLKCTRL_SSP0)|(SCB_SYSAHBCLKCTRL_IOCON);

	/* Init GPIOs */
	GPIO_GPIO0DIR=(1<<HSYNCBIT)|(1<<VSYNCBIT)|(1<<VIDEOBIT);
	GPIO_GPIO1DIR=DRAMDMASK|DRAMAMASK;
	GPIO_GPIO2DIR=(1<<LEDBIT)|(1<<DRAMGBIT)|(1<<GPIOMONBIT)|(1<<GPIOEJECTBIT)|(1<<GPIOFANBIT);
	GPIO_GPIO3DIR=(1<<DRAMRASBIT)|(1<<DRAMCASBIT)|(1<<DRAMWBIT)|(1<<GPIOHDLEDBIT);
	GPIOSET(DRAMCASPORT, DRAMCASBIT);
	GPIOSET(DRAMRASPORT, DRAMRASBIT);
	GPIOSET(DRAMWPORT, DRAMWBIT);
	GPIOSET(DRAMGPORT, DRAMGBIT);


	//Reset 'special' I/O-pins to GPIO
	IOCON_JTAG_TMS_PIO1_0=0xD1;
	IOCON_JTAG_TDO_PIO1_1=0xD1;
	IOCON_JTAG_nTRST_PIO1_2=0xD1;
	IOCON_SWDIO_PIO1_3=0xD1;

	//Enable MOSI
	IOCON_PIO0_9=1;
	//Enable HBlank pwm output (and disable tck on that pin)
	IOCON_JTAG_TCK_PIO0_10=0xD3;
	

	//Enable SSP
	SCB_PRESETCTRL=3;
//	SSP_SSP0CR0=0x01F; //TI, 16bit, /3
	SSP_SSP0CR0=0x013; //TI, 4bit, /3
	SSP_SSP0CPSR=4;
	SSP_SSP0CR1=0x2;
	SSP_SSP0DR=0xffff;

	// Enable timer16
	TMR_TMR16B0TCR=1;
	TMR_TMR16B0PR=3; //72MHz / 4 = 16MHz, close 'nuf to the pixel clock the display wants.
	TMR_TMR16B0MR0=714; //line len
	TMR_TMR16B0MR1=145; //start of pixel data
	TMR_TMR16B0MR2=400;  //hbl len
//	TMR_TMR16B0MCR=TMR_TMR16B0MCR_MR0_INT_ENABLED|TMR_TMR16B0MCR_MR0_RESET_ENABLED|TMR_TMR16B0MCR_MR1_INT_ENABLED|TMR_TMR16B0MCR_MR2_INT_ENABLED;
	TMR_TMR16B0PWMC=(1<<2); //enable hw pwm on hblank
	TMR_TMR16B0MCR=TMR_TMR16B0MCR_MR0_INT_ENABLED|TMR_TMR16B0MCR_MR0_RESET_ENABLED|TMR_TMR16B0MCR_MR1_INT_ENABLED;

//	NVIC_EnableIRQ(SSP_IRQn);
	NVIC_EnableIRQ(TIMER_16_0_IRQn);

	GPIOSET(LEDPORT, LEDBIT);
	GPIOCLEAR(VSYNCPORT, VSYNCBIT);
	GPIOCLEAR(HSYNCPORT, HSYNCBIT);

	usbInit();
	usbConnect();

	int dramCol=0;
	char i;
	const char inv[]={0x0,0x8,0x4,0xc,0x2,0xa,0x6,0xe,0x1,0x9,0x5,0xd,0x3,0xb,0x7,0xf};
	for (dramRow=0; dramRow<256; dramRow++) {
		GPIOMOD(DRAMAPORT, DRAMAMASK, dramRow<<DRAMASHIFT);
		GPIOCLEAR(DRAMRASPORT, DRAMRASBIT);
		NOP(); NOP(); NOP(); NOP();
		NOP(); NOP(); NOP(); NOP();
		for (dramCol=0; dramCol<256; dramCol++) {
/*
			if (dramCol<128) {
				i=bdc_bits[(dramCol/2)+dramRow*64];
			} else {
				i=bdc_bits[((dramCol-128)/2)+(dramRow+256)*64];
			}
			if (dramCol&1) i>>=4;
			i=inv[i&0xf];
*/
			if ((dramRow)&1) i=0xa; else i=0x5;
			GPIOMOD(DRAMDPORT, DRAMDMASK, i<<DRAMDSHIFT);
			GPIOMOD(DRAMAPORT, DRAMAMASK, dramCol<<DRAMASHIFT);
			GPIOCLEAR(DRAMWPORT, DRAMWBIT);
			NOP(); NOP(); NOP(); NOP();
			GPIOCLEAR(DRAMCASPORT, DRAMCASBIT);
			NOP(); NOP(); NOP(); NOP();
			GPIOSET(DRAMCASPORT, DRAMCASBIT);
			NOP(); NOP(); NOP(); NOP();
			GPIOSET(DRAMWPORT, DRAMWBIT);
			
			usbHandle();
		}
		GPIOSET(DRAMRASPORT, DRAMRASBIT);
	}

//	printf(" *** Program start ***\n");

	GPIOSET(GPIOFANPORT, GPIOFANBIT);
//	gpioSet(0,1); //fan
//	gpioSet(1,1); //mon

	__enable_irq();
	usbDataLen=0;

	while(1) {
		if (!(doSendLine&SENDLINE_FREETIME)) {
			while(doSendLine==0) {
				//Wait For Interrupt. This saves power and syncs up the rest of the routine quite nicely.
				__asm volatile ("WFI");
			}
			if (doSendLine&SENDLINE_BUFFER) {
				sendLine(linePos-VIDEOLINESTART);
				doSendLine&=~(SENDLINE_BUFFER);
			}
		}
		if (usbDataLen>0) {
			int x;
			int firstlast;
			int credits;
#define STARTCREDS 1
			firstlast=1;
			credits=STARTCREDS;
			while (firstlast!=2) {
				if (credits<0) firstlast|=2;
				if ((usbDataProcessed+1)==usbDataLen) firstlast|=2;
				credits-=processByte(usbData[usbDataProcessed++], firstlast);
				if (usbDataProcessed==usbDataLen) {
					firstlast=2;
					usbDataLen=0;
					usbDataProcessed=0;
				}
				firstlast&=(~1);
			}
		} else {
			usbHandle();
		}
	}
}
