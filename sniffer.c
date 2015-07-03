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
 December 09 / 2013 - by Jacek Kosek
 2015 - by Jacek Kosek
*/

//------------------------------------------------------------------------------


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "avclandrv.h"

// declaratins-----------------------------------------------------------------------

// Event types
#define EV_NOTHING	0
#define EV_STATUS	4

// Hold event Type
uint8_t Event;


void Setup();


// -------------------------------------------------------------------------------------
//	MAIN PROGRAM
//
int main() {

    Setup();

    while (1) {

        if (INPUT_IS_SET) {	 // if message from some device on AVCLan begin
            AVCLan_Read_Message();
        } else {
            // check command from HU
            if (answerReq != 0) AVCLan_SendAnswer();
        }

        // HandleEvent
        switch (Event) {
        case EV_STATUS: //recievd from timer interupt to send status
            Event &= ~EV_STATUS; //clear event
            AVCLan_Send_Status();
            break;
        }

    }
    return 0;
}


// -------------------------------------------------------------------------------------
// Setup - uP: ATMega88(p)
//
void Setup() {

    CD_ID_1 = 0x03;
    CD_ID_2 = 0x60; //was cammera

    HU_ID_1 = 0x01;
    HU_ID_2 = 0x60; //was 0x40

    MCUCR = 0; //turn on everything

//Clear Timer on Compare Mode
    TCCR1B |= (1 << WGM12 | 1 << CS12); // Configure timer 1 for CTC mode with 256 prescaler
    TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt

    AVCLan_Init();

    Event = EV_NOTHING;
    sei(); //enable global interupts

//      T * osc. freq. / prescaler - 1 (error: 1s per 50h)
    OCR1A = 1 *   14745000 /       256 - 1; // Set CTC compare value

}


//------------------------------------------------------------------------------
//Interupt Service Routine for Timer1 comparator
//Each second updating cd_Time_Sec and other...
//then set Event for updating status
ISR(TIMER1_COMPA_vect) {
    cd_Time_Sec=HexInc(cd_Time_Sec);
    if (cd_Time_Sec==0x60) {
        cd_Time_Sec = 0;
        cd_Time_Min=HexInc(cd_Time_Min);
        if (cd_Time_Min==0x60) {
            cd_Time_Min=0x0;
            cd_Disc=HexInc(cd_Disc);
        }
        cd_Track = cd_Time_Min;
    }
    if (CD_Mode==stPlay) {
        // set event to send status in main loop
        Event |= EV_STATUS;
    }
}


