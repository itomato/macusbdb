/**************************************************************************/
/*! 
    @file     drawing.c
    @author   K. Townsend (microBuilder.eu)
    @date     22 March 2010
    @version  0.10

    drawLine and drawCircle adapted from a tutorial by Leonard McMillan:
    http://www.cs.unc.edu/~mcmillan/

    drawString based on an example from Eran Duchan:
    http://www.pavius.net/downloads/tools/53-the-dot-factory

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
#include <string.h>

#include "drawing.h"

/**************************************************************************/
/*                                                                        */
/* ----------------------- Private Methods ------------------------------ */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/*!
    @brief  Draws a single bitmap character
*/
/**************************************************************************/
static void drawCharBitmap(const uint16_t xPixel, const uint16_t yPixel, uint16_t color, const uint8_t *glyph, uint8_t glyphHeightPages, uint8_t glyphWidthBits)
{
  uint16_t verticalPage, horizBit, currentY, currentX;
  uint16_t indexIntoGlyph;

  // set initial current y
  currentY = yPixel;
  currentX = xPixel;

  // for each page of the glyph
  for (verticalPage = glyphHeightPages; verticalPage > 0; --verticalPage)
  {
    // for each horizontol bit
    for (horizBit = 0; horizBit < glyphWidthBits; ++horizBit)
    {
      // next byte
      indexIntoGlyph = (glyphHeightPages * horizBit) + verticalPage - 1;
      
      currentX = xPixel + (horizBit);
      // send the data byte
      if (glyph[indexIntoGlyph] & (0X80)) drawPixel(currentX, currentY, color);
      if (glyph[indexIntoGlyph] & (0X40)) drawPixel(currentX, currentY - 1, color);
      if (glyph[indexIntoGlyph] & (0X20)) drawPixel(currentX, currentY - 2, color);
      if (glyph[indexIntoGlyph] & (0X10)) drawPixel(currentX, currentY - 3, color);
      if (glyph[indexIntoGlyph] & (0X08)) drawPixel(currentX, currentY - 4, color);
      if (glyph[indexIntoGlyph] & (0X04)) drawPixel(currentX, currentY - 5, color);
      if (glyph[indexIntoGlyph] & (0X02)) drawPixel(currentX, currentY - 6, color);
      if (glyph[indexIntoGlyph] & (0X01)) drawPixel(currentX, currentY - 7, color);
    }
    // next line of pages
    currentY += 8;
  }
}

#if defined CFG_LCD_INCLUDESMALLFONTS & CFG_LCD_INCLUDESMALLFONTS == 1
/**************************************************************************/
/*!
    @brief  Draws a single smallfont character
*/
/**************************************************************************/
static void drawCharSmall(uint16_t x, uint16_t y, uint16_t color, uint8_t c, struct FONT_DEF font)
{
  uint8_t col, column[font.u8Width];

  // Check if the requested character is available
  if ((c >= font.u8FirstChar) && (c <= font.u8LastChar))
  {
    // Retrieve appropriate columns from font data
    for (col = 0; col < font.u8Width; col++)
    {
      column[col] = font.au8FontTable[((c - 32) * font.u8Width) + col];    // Get first column of appropriate character
    }
  }
  else
  {    
    // Requested character is not available in this font ... send a space instead
    for (col = 0; col < font.u8Width; col++)
    {
      column[col] = 0xFF;    // Send solid space
    }
  }

  // Render each column
  uint16_t xoffset, yoffset;
  for (xoffset = 0; xoffset < font.u8Width; xoffset++)
  {
    for (yoffset = 0; yoffset < (font.u8Height + 1); yoffset++)
    {
      uint8_t bit = 0x00;
      bit = (column[xoffset] << (8 - (yoffset + 1)));     // Shift current row bit left
      bit = (bit >> 7);                     // Shift current row but right (results in 0x01 for black, and 0x00 for white)
      if (bit)
      {
        drawPixel(x + xoffset, y + yoffset, color);
      }
    }
  }
}
#endif

/**************************************************************************/
/*!
    @brief  Helper method to accurately draw individual circle points
*/
/**************************************************************************/
static void drawCirclePoints(int cx, int cy, int x, int y, uint16_t color)
{    
  if (x == 0) 
  {
      drawPixel(cx, cy + y, color);
      drawPixel(cx, cy - y, color);
      drawPixel(cx + y, cy, color);
      drawPixel(cx - y, cy, color);
  } 
  else if (x == y) 
  {
      drawPixel(cx + x, cy + y, color);
      drawPixel(cx - x, cy + y, color);
      drawPixel(cx + x, cy - y, color);
      drawPixel(cx - x, cy - y, color);
  } 
  else if (x < y) 
  {
      drawPixel(cx + x, cy + y, color);
      drawPixel(cx - x, cy + y, color);
      drawPixel(cx + x, cy - y, color);
      drawPixel(cx - x, cy - y, color);
      drawPixel(cx + y, cy + x, color);
      drawPixel(cx - y, cy + x, color);
      drawPixel(cx + y, cy - x, color);
      drawPixel(cx - y, cy - x, color);
  }
}

/**************************************************************************/
/*                                                                        */
/* ----------------------- Public Methods ------------------------------- */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/*!
    @brief  Draws a single pixel at the specified location

    @param[in]  x
                Horizontal position
    @param[in]  y
                Vertical position
    @param[in]  color
                Color used when drawing
*/
/**************************************************************************/
void drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  // Redirect to LCD
  lcdDrawPixel(x, y, color);
}

/**************************************************************************/
/*!
    @brief  Fills the screen with the specified color

    @param[in]  color
                Color used when drawing
*/
/**************************************************************************/
void drawFill(uint16_t color)
{
  lcdFillRGB(color);
}

/**************************************************************************/
/*!
    @brief  Draws a simple color test pattern
*/
/**************************************************************************/
void drawTestPattern(void)
{
  lcdTest();
}

#if defined CFG_LCD_INCLUDESMALLFONTS & CFG_LCD_INCLUDESMALLFONTS == 1
/**************************************************************************/
/*!
    @brief  Draws a string using a small font (6 of 8 pixels high).

    @param[in]  x
                Starting x co-ordinate
    @param[in]  y
                Starting y co-ordinate
    @param[in]  color
                Color to use when rendering the font
    @param[in]  text
                The string to render
    @param[in]  font
                Pointer to the FONT_DEF to use when drawing the string

    @section Example

    @code 

    #include "drivers/lcd/fonts/smallfonts.h"
    
    drawStringSmall(1, 210, WHITE, "5x8 System (Max 40 Characters)", Font_System5x8);
    drawStringSmall(1, 220, WHITE, "7x8 System (Max 30 Characters)", Font_System7x8);

    @endcode
*/
/**************************************************************************/
void drawStringSmall(uint16_t x, uint16_t y, uint16_t color, char* text, struct FONT_DEF font)
{
  uint8_t l;
  for (l = 0; l < strlen(text); l++)
  {
    drawCharSmall(x + (l * (font.u8Width + 1)), y, color, text[l], font);
  }
}
#endif

/**************************************************************************/
/*!
    @brief  Draws a string using the supplied font

    @param[in]  x
                Starting x co-ordinate
    @param[in]  y
                Starting y co-ordinate
    @param[in]  color
                Color to use when rendering the font
    @param[in]  fontInfo
                Pointer to the FONT_INFO to use when drawing the string
    @param[in]  str
                The string to render

    @section Example

    @code 

    #include "drivers/lcd/fonts/consolas9.h"
    
    drawString(1,   90,   GREEN,    &consolas9ptFontInfo,   "Consolas 9 (38 chars wide)");
    drawString(1,   105,  GREEN,    &consolas9ptFontInfo,   "12345678901234567890123456789012345678");

    @endcode
*/
/**************************************************************************/
void drawString(uint16_t x, uint16_t y, uint16_t color, const FONT_INFO *fontInfo, char *str)
{
  uint16_t currentX, charWidth, characterToOutput;
  const FONT_CHAR_INFO *charInfo;
  uint16_t charOffset;
  
  // set current x, y to that of requested
  currentX = x;

  // while not NULL
  while (*str != 0)
  {
    // get character to output
    characterToOutput = *str;
    
    // get char info
    charInfo = fontInfo->charInfo;
    
    // some fonts have character descriptors, some don't
    if (charInfo != NULL)
    {
      // get correct char offset
      charInfo += (characterToOutput - fontInfo->startChar);
      
      // get width from char info
      charWidth = charInfo->widthBits;
      
      // get offset from char info
      charOffset = charInfo->offset;
    }        
    else
    {
      // if no char info, char width is always 5
      charWidth = 5;
      
      // char offset - assume 5 * letter offset
      charOffset = (characterToOutput - fontInfo->startChar) * 5;
    }        
    
    // Send individual characters
    drawCharBitmap(currentX, y, color, &fontInfo->data[charOffset], fontInfo->heightPages, charWidth);

    // next char X
    currentX += charWidth + 1;
    
    // next char
    str++;
  }
}

/**************************************************************************/
/*!
    @brief  Returns the width in pixels of a string when it is rendered

    This method can be used to determine whether a string will fit
    inside a specific area, or if it needs to be broken up into multiple
    lines to be properly rendered on the screen.

    This function only applied to bitmap fonts (which can have variable
    widths).  All smallfonts (if available) are fixed width and can
    easily have their width calculated without costly functions like
    this one.

    @param[in]  fontInfo
                Pointer to the FONT_INFO for the font that will be used
    @param[in]  str
                The string that will be rendered
*/
/**************************************************************************/
uint32_t drawGetStringWidth(const FONT_INFO *fontInfo, char *str)
{
  uint32_t currChar, width = 0;
  uint32_t startChar = fontInfo->startChar;

  // until termination
  for (currChar = *str; currChar; currChar = *(++str))
  {
    // if char info exists for the font, use width from there
    if (fontInfo->charInfo != NULL)
    {
      width += fontInfo->charInfo[currChar - startChar].widthBits + 1;
    }
    else
    {
      width += 5 + 1;
    }
  }

  /* return the wdith */
  return width;
}

/**************************************************************************/
/*!
    @brief  Draws a Bresenham line

    Based on: http://www.cs.unc.edu/~mcmillan/comp136/Lecture6/Lines.html

    @param[in]  x0
                Starting x co-ordinate
    @param[in]  y0
                Starting y co-ordinate
    @param[in]  x1
                Ending x co-ordinate
    @param[in]  y1
                Ending y co-ordinate
    @param[in]  color
                Color used when drawing
*/
/**************************************************************************/
void drawLine ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color )
{
  // Check if we can used the optimised horizontal line method
  if (y0 == y1)
  {
    lcdDrawHLine(x0, x1, y0, color);
    return;
  }

  // Draw non horizontal line
  int dy = y1 - y0;
  int dx = x1 - x0;
  int stepx, stepy;

  if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
  if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
  dy <<= 1;                               // dy is now 2*dy
  dx <<= 1;                               // dx is now 2*dx

  drawPixel(x0, y0, color);
  if (dx > dy) 
  {
    int fraction = dy - (dx >> 1);        // same as 2*dy - dx
    while (x0 != x1) 
    {
      if (fraction >= 0) 
      {
        y0 += stepy;
        fraction -= dx;                   // same as fraction -= 2*dx
      }
      x0 += stepx;
      fraction += dy;                     // same as fraction -= 2*dy
      drawPixel(x0, y0, color);
    }
  } 
  else 
  {
    int fraction = dx - (dy >> 1);
    while (y0 != y1) 
    {
      if (fraction >= 0) 
      {
        x0 += stepx;
        fraction -= dy;
      }
      y0 += stepy;
      fraction += dx;
      drawPixel(x0, y0, color);
    }
  }
}

/**************************************************************************/
/*!
    @brief  Draws a circle

    Based on: http://www.cs.unc.edu/~mcmillan/comp136/Lecture7/circle.html

    @param[in]  xCenter
                The horizontal center of the circle
    @param[in]  yCenter
                The vertical center of the circle
    @param[in]  radius
                The circle's radius in pixels
    @param[in]  color
                Color used when drawing
*/
/**************************************************************************/
void drawCircle (uint16_t xCenter, uint16_t yCenter, uint16_t radius, uint16_t color)
{
  int x = 0;
  int y = radius;
  int p = (5 - radius*4)/4;

  drawCirclePoints(xCenter, yCenter, x, y, color);
  while (x < y) 
  {
    x++;
    if (p < 0) 
    {
      p += 2*x+1;
    } 
    else 
    {
      y--;
      p += 2*(x-y)+1;
    }
    drawCirclePoints(xCenter, yCenter, x, y, color);
  }
}

/**************************************************************************/
/*!
    @brief  Draws a simple (empty) rectangle

    @param[in]  x0
                Starting x co-ordinate
    @param[in]  y0
                Starting y co-ordinate
    @param[in]  x1
                Ending x co-ordinate
    @param[in]  y1
                Ending y co-ordinate
    @param[in]  color
                Color used when drawing
*/
/**************************************************************************/
void drawRectangle ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
  uint16_t x, y;

  if (y1 < y0)
  {
    // Swich y1 and y0
    y = y1;
    y1 = y0;
    y0 = y;
  }

  if (x1 < x0)
  {
    // Swich x1 and x0
    x = x1;
    x1 = x0;
    x0 = x;
  }

  drawLine (x0, y0, x1, y0, color);
  drawLine (x1, y0, x1, y1, color);
  drawLine (x1, y1, x0, y1, color);
  drawLine (x0, y1, x0, y0, color);
}

/**************************************************************************/
/*!
    @brief  Draws a filled rectangle

    @param[in]  x0
                Starting x co-ordinate
    @param[in]  y0
                Starting y co-ordinate
    @param[in]  x1
                Ending x co-ordinate
    @param[in]  y1
                Ending y co-ordinate
    @param[in]  color
                Color used when drawing
*/
/**************************************************************************/
void drawRectangleFilled ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
  int height;
  uint16_t x, y;

  if (y1 < y0)
  {
    // Swich y1 and y0
    y = y1;
    y1 = y0;
    y0 = y;
  }

  if (x1 < x0)
  {
    // Swich x1 and x0
    x = x1;
    x1 = x0;
    x0 = x;
  }

  height = y1 - y0;
  for (height = y0; y1 > height - 1; ++height)
  {
    drawLine(x0, height, x1, height, color);
  }
}

/**************************************************************************/
/*!
    @brief  Converts a 24-bit RGB color to an equivalent 16-bit RGB565 value

    @param[in]  r
                8-bit red
    @param[in]  g
                8-bit green
    @param[in]  b
                8-bit blue

    @section Example

    @code 

    // Get 16-bit equivalent of 24-bit color
    uint16_t gray = drawRGB24toRGB565(0x33, 0x33, 0x33);

    @endcode
*/
/**************************************************************************/
uint16_t drawRGB24toRGB565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

/**************************************************************************/
/*!
    @brief  Draws a progress bar with rounded corners

    @param[in]  x
                Starting x location
    @param[in]  y
                Starting y location
    @param[in]  width
                Total width of the progress bar in pixels
    @param[in]  borderColor
                16-bit color for the outer border
    @param[in]  fillColor
                16-bit color for the outer border fill
    @param[in]  barBorderColor
                16-bit color for the inner bar's border
    @param[in]  barFillColor
                16-bit color for the inner bar's fill
    @param[in]  progress
                Progress percentage (between 0 and 100)

    @section Example

    @code 

    // Get 16-bit equivalent of 24-bit color
    uint16_t gray = drawRGB24toRGB565(0x33, 0x33, 0x33);

    // Draw a the progress bar
    drawProgressBar(10, 200, 100, 20, WHITE, gray, WHITE, BLUE, 90);

    @endcode
*/
/**************************************************************************/
void drawProgressBar ( uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t borderColor, uint16_t fillColor, uint16_t barBorderColor, uint16_t barFillColor, uint8_t progress )
{
  // Draw border with rounded corners
  drawLine(x+2, y, x + width - 2, y, borderColor);
  drawLine(x + width, y + 2, x + width, y + height - 2, borderColor);
  drawLine(x + width - 2, y + height, x + 2, y + height, borderColor);
  drawLine(x, y + height - 2, x, y + 2, borderColor);
  drawPixel(x + 1, y + 1, borderColor);
  drawPixel(x + width - 1, y + 1, borderColor);
  drawPixel(x + 1, y + height - 1, borderColor);
  drawPixel(x + width - 1, y + height - 1, borderColor);

  // Fill outer container
  drawLine(x+1, y+2, x+1, y+height-2, fillColor);
  drawLine(x+2, y+height-1, x+width-2, y+height-1, fillColor);
  drawLine(x+width-1, y+height-2, x+width-1, y+2, fillColor);
  drawLine(x+width-2, y+1, x+2, y+1, fillColor);
  drawRectangleFilled(x+2, y+2, x+width-2, y+height-2, fillColor);

  // Progress bar
  if (progress > 0 && progress <= 100)
  {
    // Calculate bar size
    uint16_t bw;
    bw = (width - 6);   // bar at 100%
    if (progress != 100)
    {
      bw = (bw * progress) / 100;
    } 
    drawRectangle(x + 3, y + 3, bw + x + 3, y + height - 3, barBorderColor);
    drawRectangleFilled(x + 4, y + 4, bw + x + 3 - 1, y + height - 4, barFillColor);
  }
}

#ifdef CFG_SDCARD
/**************************************************************************/
/*!
    @brief  Loads an image from an SD card and renders it
*/
/**************************************************************************/
void drawImageFromFile(uint16_t x, uint16_t y, char *filename)
{
  lcdDrawImageFromFile(x, y, filename);
}
#endif

/**************************************************************************/
/*!
    @brief  Renders a bitmap image
*/
/**************************************************************************/
void drawImage(uint16_t x, uint16_t y, uint16_t *data)
{
  lcdDrawImage(x, y, data);
}


