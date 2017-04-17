/**************************************************************************/
/*! 
    @file     ILI9325.c
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
#include "ILI9325.h"

/*************************************************/
/* Private Methods                               */
/*************************************************/

/*************************************************/
void ili9325Delay(unsigned int t)
{
  unsigned char t1;
  while(t--)
  for ( t1=10; t1 > 0; t1-- )
  {
    __asm("nop");
  }
}

/*************************************************/
void ili9325WriteCmd(uint16_t command)
{
  CLR_CS_CD;      // Saves 7 commands compared to "CLR_CS; CLR_CD;"
  SET_RD_WR;      // Saves 7 commands compared to "SET_RD; SET_WR;"
  ILI9325_GPIO2DATA_DATA = (command >> (8 - ILI9325_DATA_OFFSET));
  CLR_WR;
  SET_WR;
  ILI9325_GPIO2DATA_DATA = command << ILI9325_DATA_OFFSET;
  CLR_WR;
  SET_WR_CS;      // Saves 7 commands compared to "SET_WR; SET_CS;"
}

/*************************************************/
void ili9325WriteData(uint16_t data)
{
  CLR_CS;
  SET_CD_RD_WR;   // Saves 14 commands compared to "SET_CD; SET_RD; SET_WR"
  ILI9325_GPIO2DATA_DATA = (data >> (8 - ILI9325_DATA_OFFSET));
  CLR_WR;
  SET_WR;
  ILI9325_GPIO2DATA_DATA = data << ILI9325_DATA_OFFSET;
  CLR_WR;
  SET_WR_CS;      // Saves 7 commands compared to "SET_WR, SET_CS;"
}

/*************************************************/
void ili9325Command(uint16_t command, uint16_t data)
{
  // Provided for convenience sake ... shouldn't be used
  // in critical sections since it adds an extra
  // branch, etc.
  ili9325WriteCmd(command);
  ili9325WriteData(data);
}

uint16_t ili9325BGR2RGB(uint16_t color)   
{   
  uint16_t r, g, b;   
   
  b = (color>>0)  & 0x1f;   
  g = (color>>5)  & 0x3f;   
  r = (color>>11) & 0x1f;   
     
  return( (b<<11) + (g<<5) + (r<<0) );
}  

/***********************************************************
  Read pixel from LCD controller at current position
***********************************************************/
uint16_t ili9325Read(void)
{
  // ToDo

  return 0;
}

/*************************************************/
void ili9325SetCursor(uint16_t x, uint16_t y)
{
  ili9325Command(0x0020, x-1);       // GRAM Address Set (Horizontal Address) (R20h)
  ili9325Command(0x0021, y-1);       // GRAM Address Set (Vertical Address) (R21h)
}

/*************************************************/
static void ili9325InitDisplay(void)
{
  // Clear data line
  GPIO_GPIO2DATA &= ~ILI9325_DATA_MASK;
    
  SET_RD;
  SET_WR;
  SET_CS;
  SET_CD;

  // Reset display
  CLR_RESET;
  ili9325Delay(10000);
  SET_RESET;
  ili9325Delay(500);

  ili9325Command(0x00FF, 0x0001);
  ili9325Command(0x00F3, 0x0008);
  ili9325WriteCmd(0x00F3);

  ili9325Command(0x0001, 0x0100);     // Driver Output Control Register (R01h)
  ili9325Command(0x0002, 0x0700);     // LCD Driving Waveform Control (R02h)
  ili9325Command(0x0003, 0x1030);     // Entry Mode (R03h)  
  ili9325Command(0x0008, 0x0302);
  ili9325Command(0x0009, 0x0000);
  ili9325Command(0x0010, 0x0000);     // Power Control 1 (R10h)
  ili9325Command(0x0011, 0x0007);     // Power Control 2 (R11h)  
  ili9325Command(0x0012, 0x0000);     // Power Control 3 (R12h)
  ili9325Command(0x0013, 0x0000);     // Power Control 4 (R13h)
  ili9325Delay(1000);  
  ili9325Command(0x0010, 0x14B0);     // Power Control 1 (R10h)  
  ili9325Delay(500);  
  ili9325Command(0x0011, 0x0007);     // Power Control 2 (R11h)  
  ili9325Delay(500);  
  ili9325Command(0x0012, 0x008E);     // Power Control 3 (R12h)
  ili9325Command(0x0013, 0x0C00);     // Power Control 4 (R13h)
  ili9325Command(0x0029, 0x0015);     // NVM read data 2 (R29h)
  ili9325Delay(500);
  ili9325Command(0x0030, 0x0000);     // Gamma Control 1
  ili9325Command(0x0031, 0x0107);     // Gamma Control 2
  ili9325Command(0x0032, 0x0000);     // Gamma Control 3
  ili9325Command(0x0035, 0x0203);     // Gamma Control 6
  ili9325Command(0x0036, 0x0402);     // Gamma Control 7
  ili9325Command(0x0037, 0x0000);     // Gamma Control 8
  ili9325Command(0x0038, 0x0207);     // Gamma Control 9
  ili9325Command(0x0039, 0x0000);     // Gamma Control 10
  ili9325Command(0x003C, 0x0203);     // Gamma Control 13
  ili9325Command(0x003D, 0x0403);     // Gamma Control 14
  ili9325Command(0x0050, 0x0000);     // Window Horizontal RAM Address Start (R50h)
  ili9325Command(0x0051, 0x00EF);     // Window Horizontal RAM Address End (R51h)
  ili9325Command(0x0052, 0X0000);     // Window Vertical RAM Address Start (R52h)
  ili9325Command(0x0053, 0x013F);     // Window Vertical RAM Address End (R53h)
  ili9325Command(0x0060, 0xa700);     // Driver Output Control (R60h)
  ili9325Command(0x0061, 0x0001);     // Driver Output Control (R61h)
  ili9325Command(0x0090, 0X0029);     // Panel Interface Control 1 (R90h)

  // Display On
  ili9325Command(0x0007, 0x0133);     // Display Control (R07h)
  ili9325Delay(500);
  ili9325WriteCmd(0x0022);
}

/*************************************************/
void ili9325Home(void)
{
  ili9325Command(0x0020, 0X0000);     // GRAM Address Set (Horizontal Address) (R20h)
  ili9325Command(0x0021, 0X0000);     // GRAM Address Set (Vertical Address) (R21h)
  ili9325WriteCmd(0x0022);            // Write Data to GRAM (R22h)
}

/*************************************************/
void ili9325SetWindow(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1)
{
  ili9325Command(0x0050, x-1);       // Window Horizontal RAM Address Start (R50h)
  ili9325Command(0x0051, x1-1);      // Window Horizontal RAM Address End (R51h)
  ili9325Command(0x0052, y-1);       // Window Vertical RAM Address Start (R52h) )
  ili9325Command(0x0053, y1-1);      // Window Vertical RAM Address End (R53h)
}

/*************************************************/
/* Public Methods                                */
/*************************************************/

/*************************************************/
void lcdInit(void)
{
  // Set control line pins to output
  gpioSetDir(ILI9325_CS_PORT, ILI9325_CS_PIN, 1);
  gpioSetDir(ILI9325_CD_PORT, ILI9325_CD_PIN, 1);
  gpioSetDir(ILI9325_WR_PORT, ILI9325_WR_PIN, 1);
  gpioSetDir(ILI9325_RD_PORT, ILI9325_RD_PIN, 1);
  
  // Set data port pins to output
  ILI9325_GPIO2DATA_SETOUTPUT;

  // Disable pullups
  gpioSetPullup(&IOCON_PIO2_1, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_2, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_3, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_4, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_5, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_6, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_7, gpioPullupMode_Inactive);
  gpioSetPullup(&IOCON_PIO2_8, gpioPullupMode_Inactive);
  
  // Set backlight
  gpioSetDir(ILI9325_BL_PORT, ILI9325_BL_PIN, 1);      // set to output
  gpioSetValue(ILI9325_BL_PORT, ILI9325_BL_PIN, 0);    // turn on

  // Initialize the display
  ili9325InitDisplay();

  // Fill black
  lcdFillRGB(BLACK);
}

/*************************************************/
void lcdTest(void)
{
  uint32_t i,j;
  ili9325Home();
  
  for(i=0;i<320;i++)
  {
    for(j=0;j<240;j++)
    {
      if(i>279)ili9325WriteData(WHITE);
      else if(i>239)ili9325WriteData(BLUE);
      else if(i>199)ili9325WriteData(GREEN);
      else if(i>159)ili9325WriteData(CYAN);
      else if(i>119)ili9325WriteData(RED);
      else if(i>79)ili9325WriteData(MAGENTA);
      else if(i>39)ili9325WriteData(YELLOW);
      else ili9325WriteData(BLACK);
    }
  }
}

/*************************************************/
void lcdFillRGB(uint16_t data)
{
  unsigned int i;
  ili9325Home();
  
  uint32_t pixels = 320*240;
  for ( i=0; i < pixels; i++ )
  {
    ili9325WriteData(data);
  } 
}

/*************************************************/
void lcdDrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  ili9325WriteCmd(0x0020); // GRAM Address Set (Horizontal Address) (R20h)
  ili9325WriteData(x-1);
  ili9325WriteCmd(0x0021); // GRAM Address Set (Vertical Address) (R21h)
  ili9325WriteData(y-1);
  ili9325WriteCmd(0x0022);  // Write Data to GRAM (R22h)
  ili9325WriteData(color);
}

/*************************************************/
void lcdDrawHLine(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color)
{
  // Allows for slightly better performance than setting individual pixels
  uint16_t x, pixels;

  if (x1 < x0)
  {
    // Switch x1 and x0
    x = x1;
    x1 = x0;
    x0 = x;
  }

  ili9325WriteCmd(0x0020); // GRAM Address Set (Horizontal Address) (R20h)
  ili9325WriteData(x0-1);
  ili9325WriteCmd(0x0021); // GRAM Address Set (Vertical Address) (R21h)
  ili9325WriteData(y-1);
  ili9325WriteCmd(0x0022);  // Write Data to GRAM (R22h)
  for (pixels = 0; pixels < x1 - x0 + 1; pixels++)
  {
    ili9325WriteData(color);
  }
}

#ifdef CFG_SDCARD
/**************************************************************************/
/*!
    @brief  Loads an image from an SD card and renders it
*/
/**************************************************************************/
void lcdDrawImageFromFile(uint16_t x, uint16_t y, char *filename)
{
}
#endif

/**************************************************************************/
/*!
    @brief  Renders a bitmap image
*/
/**************************************************************************/
void lcdDrawImage(uint16_t x, uint16_t y, const uint16_t *data)
{
  // data[0] = Width
  // data[1] = height
  // data[2] = Compression
  // data[3] = Compression Window
  // data[4] = Pixel Data ...

  uint16_t line, width;
  uint32_t currentPixel, totalPixels;
  
  width = 0;
  
  // Calculate total pixels (width * height)
  totalPixels = data[0] * data[1];
  currentPixel = 0;

  // Draw individual pixels
  for (line = 0; line < data[1]; line++)
  {
    // Set cursor to starting x, y position
    ili9325SetCursor(x, y + line);
    // Prepare to write data to GRAM
    ili9325WriteCmd(0x0022);  // Write Data to GRAM (R22h)
    do 
    {
      ili9325WriteData(data[currentPixel]);
      currentPixel++;
    } while (--width);
    width = data[0];          // Reset width counter
  }
}

/*************************************************/
uint16_t lcdGetPixel(uint16_t x, uint16_t y)
{
  ili9325SetCursor(x, y);
  ili9325WriteCmd(0x0022);
  return (ili9325Read());
}
