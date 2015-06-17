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


#ifndef __AVCLANDRV_H
#define __AVCLANDRV_H
//------------------------------------------------------------------------------
#include <inttypes.h>

// Public Macro
// true if message from some device on AVCLan begin
#define INPUT_IS_SET (ACSR & _BV(ACO))

/* Private macro */
#define INPUT_IS_CLEAR (!(ACSR & _BV(ACO)))

// Private timers interupt on off to be checked
#define STOPEvent  cbi(TIMSK1, TOIE1); cbi(UCSR0B, RXCIE0);
// Private timers interupt on off to be checked
#define STARTEvent sbi(TIMSK1, TOIE1); sbi(UCSR0B, RXCIE0);

// Not used might be useful
#define CHECK_AVC_LINE	if (INPUT_IS_SET) AVCLan_Read_Message();

// Private: block line eg. you can't read
void AVC_HoldLine();
// Private: (not used in main program)
void AVC_ReleaseLine();

#define MAXMSGLEN	32

// Head Unid ID 1
uint8_t HU_ID_1;		// 0x01
// Head Unid ID 2
uint8_t HU_ID_2;		// 0x40

// Device ID 1
uint8_t CD_ID_1;		// 0x03
// Device ID 2
uint8_t CD_ID_2;		// 0x60

// Commands:
#define cmNull		0
#define cmStatus1	1
#define cmStatus2	2
#define cmStatus3	3
#define cmStatus4	4

// JKK public: used in sniffer.c
#define cmRegister	100
#define cmInit		101
#define cmCheck		102
#define cmPlayIt	103
#define cmBeep		110

#define cmNextTrack	120
#define cmPrevTrack	121
#define cmNextDisc	122
#define cmPrevDisc	123

#define cmScanModeOn	130
#define cmScanModeOff	131

#define cmPlayReq1	5
#define cmPlayReq2	6
#define cmPlayReq3	7
#define cmStopReq	8
#define cmStopReq2	9

typedef enum { stStop=0, stPlay=1 } cd_modes;
cd_modes CD_Mode;

// Place where whole message is stored during reading
uint8_t broadcast;
uint8_t master1;
uint8_t master2;
uint8_t slave1;
uint8_t slave2;
uint8_t message_len;
uint8_t message[MAXMSGLEN];


uint8_t data_control;
uint8_t data_len;
uint8_t data[MAXMSGLEN];

/* Public function AVCLan_Read_Message()
 Read the message, store it, process it, prepare answer.
 Return: 1 - OK
	 0 - error
 STEPS:
 1. stop interup from timer (TODO: might be colision)
 2. check start bit if too long: T1 if too short T2
 3. save to "brodcast"
	    master1 & 2
	    slave1 & 2
 4. if fro_me send ACK each time
 5. save to message_len lenght
 6. save to message[]
 7. turn interupt again
 8. if for_me what kind of request is needed write in answerReq as cm....
 9. if not brodcast checked and answer is prepared*/
uint8_t AVCLan_Read_Message();

/* send information about status cd track time */
void AVCLan_Send_Status();

// Initialize AVCLan
void AVCLan_Init();

/* Private */
void AVCLan_Register();

/*  Public function uint8_t AVCLan_SendData()
  Sends data using the same variables:
  master1 & 2
  slave1 & 2
  message_len
  message[] */
uint8_t  AVCLan_SendData();

/* Public function uint8_t AVCLan_SendAnswer()
Send right answer for requested command stored in answerReq
mostly it use AVCLan_SendAnswerFrame which usually forward return
from one of this this functions:
  AVCLan_SendDataBroadcast();
  AVCLan_SendData;*/
uint8_t  AVCLan_SendAnswer();

/*  Public function uint8_t AVCLan_SendData()
  Sends data using the same variables:
  master1 & 2
  slave1 & 2
  message_len
  message[] */
uint8_t  AVCLan_SendDataBroadcast();

// Send command eg: cmBeep
uint8_t  AVCLan_Command(uint8_t command);


uint8_t  HexInc(uint8_t data);
uint8_t  HexDec(uint8_t data);
uint8_t  Dec2Toy(uint8_t data);

uint8_t check_timeout;

uint8_t cd_Disc; // JKK public: used in sniffer.c
uint8_t cd_Track;
uint8_t cd_Time_Min;
uint8_t cd_Time_Sec;

uint8_t playMode;

// Public: Function send message
uint8_t AVCLan_SendMyData(uint8_t *data_tmp, uint8_t s_len);
// Public: Function send brodcast massage
uint8_t AVCLan_SendMyDataBroadcast(uint8_t *data_tmp, uint8_t s_len);

void ShowInMessage();
void ShowOutMessage();

//------------------------------------------------------------------------------

// Public: if answer requested
uint8_t answerReq;
//------------------------------------------------------------------------------
#endif
