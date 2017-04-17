/**************************************************************************/
/*! 
    @file     sysinit.c
    @author   K. Townsend (microBuilder.eu)
    @date     22 March 2010
    @version  0.10

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2010, microBuilder SARL
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "sysinit.h"

#include "core/cpu/cpu.h"
#include "core/pmu/pmu.h"

#ifdef CFG_PRINTF_UART
  #include "core/uart/uart.h"
#endif

#ifdef CFG_INTERFACE
  #include "core/cmd/cmd.h"
#endif

#ifdef CFG_CHIBI
  #include "drivers/chibi/chb.h"
  static chb_rx_data_t rx_data;
#endif

#ifdef CFG_USBHID
  #include "core/usbhid-rom/usbhid.h"
#endif

#ifdef CFG_USBCDC
  #include "core/usbcdc/usb.h"
  #include "core/usbcdc/usbcore.h"
  #include "core/usbcdc/usbhw.h"
  #include "core/usbcdc/cdcuser.h"
#endif

#ifdef CFG_LCD
  #include "drivers/lcd/lcd.h"
  #include "drivers/lcd/drawing.h"
  #include "drivers/lcd/fonts/consolas9.h"
  #include "drivers/lcd/fonts/consolas11.h"
  #include "drivers/lcd/fonts/consolas16.h"
  #include "drivers/lcd/fonts/smallfonts.h"
#endif

#ifdef CFG_I2CEEPROM
  #include "drivers/eeprom/mcp24aa/mcp24aa.h"
#endif

#ifdef CFG_SDCARD
  #include "core/ssp/ssp.h"
  #include "drivers/fatfs/diskio.h"
  #include "drivers/fatfs/ff.h"
  static FILINFO Finfo;
  static FATFS Fatfs[1];
  static uint8_t buf[64];
#endif

/**************************************************************************/
/*! 
    Configures the core system clock and sets up any mandatory
    peripherals like the systick timer, UART for printf, etc.

    This function should set the HW to the default state you wish to be
    in coming out of reset/startup, such as disabling or enabling LEDs,
    setting specific pin states, etc.
*/
/**************************************************************************/
void systemInit()
{
  // Setup the cpu and core clock
  cpuInit();

  // Initialise the systick timer (delay set in projectconfig.h)
//  systickInit(CFG_SYSTICK_DELAY_IN_MS);

  /* Enable AHB clock to the GPIO domain. */
  SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_GPIO);

}


#if 0
/**************************************************************************/
/*! 
    @brief Sends a single byte to a pre-determined peripheral (UART, etc.).

    @param[in]  byte
                Byte value to send
*/
/**************************************************************************/
void __putchar(const char c) 
{
  #ifdef CFG_PRINTF_UART
    // Send output to UART
    uartSendByte(c);
  #endif

  #ifdef CFG_PRINTF_USBCDC
    usbcdcSendByte(c);
  #endif

  #if defined CFG_PRINTF_NONE
    // Ignore output
  #endif
}

/**************************************************************************/
/*! 
    @brief Sends a string to a pre-determined end point (UART, etc.).

    @param[in]  str
                Text to send
*/
/**************************************************************************/
int puts(const char * str)
{
  while(*str) __putchar(*str++);
  return 0;
}

#endif