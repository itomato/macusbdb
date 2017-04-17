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

extern unsigned char fontdata_8x8[];
extern void dramSetByte(int x, int y, char byte);


/**************************************************************************/
/*! 
    @brief Sends a single byte to a pre-determined peripheral (UART, etc.).

    @param[in]  byte
                Byte value to send
*/
/**************************************************************************/
void __putchar(const char c) {
	static int x=0;
	static int y=0;
	if (c=='\n') {
		x=0;
		y+=8;
	} else {
		int i;
		for (i=0; i<8; i++) {
			dramSetByte(x,y+i,fontdata_8x8[c*8+i]);
		}
		x+=1;
	}
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


