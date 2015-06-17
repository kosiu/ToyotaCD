/*
  Copyright (C) 2006 Marcin Slonicki <marcin@softservice.com.pl>.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 -----------------------------------------------------------------------
	this file is a part of the TOYOTA Corolla MP3 Player Project
 -----------------------------------------------------------------------
 		http://www.softservice.com.pl/corolla/avc

 May 28 / 2009	- version 2

*/

#ifndef __COM232_H
#define __COM232_H
//------------------------------------------------------------------------------

#include <inttypes.h>

//------------------------------------------------------------------------------

uint8_t RS232_RxCharBuffer[25], RS232_RxCharBegin, RS232_RxCharEnd;
uint8_t readkey;
//------------------------------------------------------------------------------

void RS232_Init(void);
void RS232_S(uintptr_t str_addr);
void RS232_SendByte(uint8_t Data);
void RS232_Print(char* pBuf);
void RS232_PrintHex4(uint8_t Data);
void RS232_PrintHex8(uint8_t Data);
void RS232_PrintDec(uint8_t Data);
void RS232_PrintDec2(uint8_t Data);

//------------------------------------------------------------------------------
// LED
#define LED_ON()	cbi(PORTB, 0)
#define LED_OFF()	sbi(PORTB, 0)

//------------------------------------------------------------------------------
#endif
