/**************************************************************************/
/*! 
    @file     usbhid.c
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

#include "core/usbhid-rom/usb.h"
#include "core/usbhid-rom/usbconfig.h"
#include "core/usbhid-rom/rom_drivers.h"
#include "core/gpio/gpio.h"

#include "usbhid.h"

USB_DEV_INFO DeviceInfo;
HID_DEVICE_INFO HidDevInfo;
ROM ** rom = (ROM **)0x1fff1ff8;

/**************************************************************************/
/*! 
    @brief Gets the HID In Report (the report going from the LPC1343 to
    the USB host)
*/
/**************************************************************************/
void usbHIDGetInReport (uint8_t src[], uint32_t length)
{
  uint8_t PCInReportData = 0;
  
  // ToDo: Move this logic elsewhere to keep code 'generic'

  // If LED enabled set bit 0 to 1
  if (!gpioGetValue(CFG_LED_PORT, CFG_LED_PIN))
  {
    PCInReportData |= 1<<0;
  }

  src[0] = PCInReportData;
}

/**************************************************************************/
/*! 
    @brief Sets the HID Out Report (the report coming in from the USB
    host to the LPC1343).
*/
/**************************************************************************/
void usbHIDSetOutReport (uint8_t dst[], uint32_t length)
{
  uint8_t PCOutReportData = dst[0];
  // Check bit 0 in the incoming report to determine is LED should
  // be enabled or disabled (1 = enabled, 0 = disabled)
  if (PCOutReportData & (1<<0))
  {
    // Enable LED (set low)
    gpioSetValue (CFG_LED_PORT, CFG_LED_PIN, 0);
  }
  else
  {
    // Disable LED (set high)
    gpioSetValue (CFG_LED_PORT, CFG_LED_PIN, 1);
  }
}

/**************************************************************************/
/*! 
    @brief Initialises the USB port

    The ROM-based USB HID code is capable of configuring the PLL and pins
    for USB, but there seems to be a bug in the code that sets the system
    clock to 48MHz (normally the USB and System clocks can be configured
    seperately).  As such, this code does not use the "init_clk_pins()"
    function in the rom, and the USB clock and pins are manually
    configured.
*/
/**************************************************************************/
void usbHIDInit (void)
{
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

  // Set USB pin functions
  IOCON_PIO0_1 &= ~IOCON_PIO0_1_FUNC_MASK;
  IOCON_PIO0_1 |= IOCON_PIO0_1_FUNC_CLKOUT;           // CLK OUT
  IOCON_PIO0_3 &= ~IOCON_PIO0_3_FUNC_MASK;
  IOCON_PIO0_3 |= IOCON_PIO0_3_FUNC_USB_VBUS;         // VBus
  IOCON_PIO0_6 &= ~IOCON_PIO0_6_FUNC_MASK;
  IOCON_PIO0_6 |= IOCON_PIO0_6_FUNC_USB_CONNECT;      // Soft Connect

  // Disable internal resistor on VBUS (0.3)
  gpioSetPullup(&IOCON_PIO0_3, gpioPullupMode_Inactive);

  // HID Device Info
  volatile int n;
  HidDevInfo.idVendor = USB_VENDOR_ID;
  HidDevInfo.idProduct = USB_PROD_ID;
  HidDevInfo.bcdDevice = USB_DEVICE; 
  HidDevInfo.StrDescPtr = (uint32_t)&USB_HIDStringDescriptor[0];
  #ifdef CFG_USBHID_EXAMPLE  
  HidDevInfo.InReportCount = 2;
  HidDevInfo.OutReportCount = 2;
  #else
  HidDevInfo.InReportCount = 1;
  HidDevInfo.OutReportCount = 1;
  #endif
  HidDevInfo.SampleInterval = 0x20;
  HidDevInfo.InReport = usbHIDGetInReport;
  HidDevInfo.OutReport = usbHIDSetOutReport;

  DeviceInfo.DevType = USB_DEVICE_CLASS_HUMAN_INTERFACE;
  DeviceInfo.DevDetailPtr = (uint32_t)&HidDevInfo;
  
  /* Enable Timer32_1, IOCON, and USB blocks (for USB ROM driver) */
  SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B1 | SCB_SYSAHBCLKCTRL_IOCON | SCB_SYSAHBCLKCTRL_USB_REG);

  /* Use pll and pin init function in rom */
  /* Warning: This will also set the system clock to 48MHz! */
  // (*rom)->pUSBD->init_clk_pins();   

  /* insert a delay between clk init and usb init */
  for (n = 0; n < 75; n++) {__asm("nop");}

  (*rom)->pUSBD->init(&DeviceInfo); /* USB Initialization */
  (*rom)->pUSBD->connect(TRUE);     /* USB Connect */
}

/**************************************************************************/
/*! 
    @brief Passes the USB interrupt to the internal ROM-based handler
*/
/**************************************************************************/
#ifdef CFG_USBHID
void USB_IRQHandler()
{
  (*rom)->pUSBD->isr();
}
#endif

