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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "timer.h"
#include "avclandrv.h"
#include "com232.h"
#include "const.h"
#include "logger.h"


//------------------------------------------------------------------------------
// Preprocesor Definitions

#define AVC_OUT_EN()	sbi(PORTD, 6); sbi(DDRD, 6);  sbi(DDRD, 7); sbi(ACSR, ACD); 
#define AVC_OUT_DIS()	cbi(PORTD, 6); cbi(DDRD, 6);  cbi(DDRD, 7); cbi(ACSR, ACD);
#define AVC_SET_1()  	sbi(PORTD, 6);
#define AVC_SET_0()  	cbi(PORTD, 6);

/* Private macro */
#define INPUT_IS_CLEAR (!(ACSR & _BV(ACO)))

// Private timers interupt on off to be checked
#define STOPEvent  cbi(TIMSK1, TOIE1); cbi(UCSR0B, RXCIE0);
// Private timers interupt on off to be checked
#define STARTEvent sbi(TIMSK1, TOIE1); sbi(UCSR0B, RXCIE0);

// Not used might be useful
#define CHECK_AVC_LINE	if (INPUT_IS_SET) AVCLan_Read_Message();


//------------------------------------------------------------------------------
// Global varibles Definitions

uint8_t CD_ID_1;
uint8_t CD_ID_2;

uint8_t HU_ID_1;
uint8_t HU_ID_2;

uint8_t cd_Disc;
uint8_t cd_Track;
uint8_t cd_Time_Min;
uint8_t cd_Time_Sec;

uint8_t playMode;

// if answer requested
uint8_t answerReq;


//Not in the header----------------------------------
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

uint8_t parity_bit;

uint8_t repeatMode;
uint8_t randomMode;

// we need check answer (to avclan check) timeout
// when is more then 1 min, FORCE answer.
uint8_t check_timeout;



//---------------------------------------------------
#define SW_ID	0x25 //was 0x12

// commands
const uint8_t stat1[]       = { 0x4, 0x00, 0x00, 0x01, 0x0A };
const uint8_t stat2[]       = { 0x4, 0x00, 0x00, 0x01, 0x08 };
const uint8_t stat3[]       = { 0x4, 0x00, 0x00, 0x01, 0x0D };
const uint8_t stat4[]       = { 0x4, 0x00, 0x00, 0x01, 0x0C };
// broadcast
const uint8_t lan_stat1[]   = { 0x3, 0x00, 0x01, 0x0A };
const uint8_t lan_reg[]     = { 0x3,SW_ID, 0x01, 0x00 };
const uint8_t lan_init[]    = { 0x3,SW_ID, 0x01, 0x01 };
const uint8_t lan_check[]   = { 0x3,SW_ID, 0x01, 0x20 };
const uint8_t lan_playit[]  = { 0x4,SW_ID, 0x01, 0x45, 0x63 };
const uint8_t play_req1[]   = { 0x4, 0x00, 0x25, 0x63, 0x80 };
#ifdef __AVENSIS__
const uint8_t play_req2[]   = { 0x6, 0x00,SW_ID, 0x63, 0x42 };
#else
const uint8_t play_req2[]   = { 0x6, 0x00,SW_ID, 0x63, 0x42, 0x01, 0x00 };
#endif
const uint8_t play_req3[]   = { 0x6, 0x00,SW_ID, 0x63, 0x42, 0x41, 0x00 };
const uint8_t stop_req[]    = { 0x5, 0x00,SW_ID, 0x63, 0x43, 0x01 };
const uint8_t stop_req2[]   = { 0x5, 0x00,SW_ID, 0x63, 0x43, 0x41 };
const uint8_t btn_scan[]    = { 0x4, 0x00, 0x25, 0x63, 0x95 };
// answers
const uint8_t CMD_REGISTER[]= { 0x1, 0x05, 0x00, 0x01,SW_ID, 0x10, 0x63 };
const uint8_t CMD_STATUS1[] = { 0x1, 0x04, 0x00, 0x01, 0x00, 0x1A };
const uint8_t CMD_STATUS2[] = { 0x1, 0x04, 0x00, 0x01, 0x00, 0x18 };
const uint8_t CMD_STATUS3[] = { 0x1, 0x04, 0x00, 0x01, 0x00, 0x1D };
const uint8_t CMD_STATUS4[] = { 0x1, 0x05, 0x00, 0x01, 0x00, 0x1C, 0x00 };
      uint8_t CMD_CHECK[]   = { 0x1, 0x06, 0x00, 0x01,SW_ID, 0x30, 0x00, 0x00 };
const uint8_t CMD_STATUS5[] = { 0x1, 0x05, 0x00, 0x5C, 0x12, 0x53, 0x02 };
const uint8_t CMD_STATUS5A[]= { 0x0, 0x05, 0x5C, 0x31, 0xF1, 0x00, 0x00 };
const uint8_t CMD_STATUS6[] = { 0x1, 0x06, 0x00, 0x5C, 0x32, 0xF0, 0x02, 0x00 };
const uint8_t CMD_PLAY_OK1[]= { 0x1, 0x05, 0x00, 0x63,SW_ID, 0x50, 0x01 };
const uint8_t CMD_PLAY_OK2[]= { 0x1, 0x05, 0x00, 0x63,SW_ID, 0x52, 0x01 };
const uint8_t CMD_PLAY_OK3[]= { 0x0, 0x0B, 0x63, 0x31, 0xF1, 0x01, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0x80 };
      uint8_t CMD_PLAY_OK4[]= { 0x0, 0x0B, 0x63, 0x31, 0xF1, 0x01, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 };
const uint8_t CMD_STOP1[]   = { 0x1, 0x05, 0x00, 0x63,SW_ID, 0x53, 0x01 };
      uint8_t CMD_STOP2[]   = { 0x0, 0x0B, 0x63, 0x31, 0xF1, 0x00, 0x30, 0x00, 0x00,0x00, 0x00, 0x00, 0x80 };
const uint8_t CMD_BEEP[]    = { 0x1, 0x05, 0x00, 0x63, 0x29, 0x60, 0x02 };

//------------------------------------------------------------------------------
// Functions Declaration

// Private: block line eg. you can't read
void AVC_HoldLine();
// Private: (not used in main program)
void AVC_ReleaseLine();


void ShowInMessage();
void ShowOutMessage();

// compare what is in the global varibale message[] with comand from argument
// return:
// 1 - the same
// 0 - difference
uint8_t CheckCmd(uint8_t *cmd);

// puts command from argument to global variable data and send brodcasted or not
// using: AVCLan_SendData() or AVCLan_SendDataBroadcast()
uint8_t AVCLan_SendAnswerFrame(uint8_t *cmd);

// this is kind of the test sends different commands and chack if there is any answer
uint8_t AVCLan_SendInitCommands();

uint8_t  HexDec(uint8_t data);
uint8_t  Dec2Toy(uint8_t data);

// more atomic functions (hardware layer)
void AVC_HoldLine(); 				     // Low Level function
void AVC_ReleaseLine(); 			     // Low Level function
uint8_t AVCLan_Read_Byte(uint8_t length);	     // Low Level function
uint8_t AVCLan_Send_StartBit(); 		     // Low Level function
void AVCLan_Send_Bit1(); 			     // Low Level function
void AVCLan_Send_Bit0(); 			     // Low Level function
uint8_t AVCLan_Read_ACK(); 			     // Low Level function
uint8_t AVCLan_Send_ACK(); 			     // Low Level function
uint8_t AVCLan_Send_Byte(uint8_t byte, uint8_t len); // Low Level function
uint8_t AVCLan_Send_ParityBit(); 		     // Low Level function

//------------------------------------------------------------------------------
// Low Level function
void AVC_HoldLine()
{
 STOPEvent;

 // wait for free line
 uint8_t T=0;
 uint8_t line_busy = 1;

 timer0_source(CK64);
 timer0_start();
 do {
 	while (INPUT_IS_CLEAR) {
		T = TCNT0;
		if (T >= 25) break;
 	}
 	if (T > 24) line_busy=0;
 } while (line_busy);

 // switch to out mode
 AVC_OUT_EN();
 AVC_SET_1();

 STARTEvent;
}
//------------------------------------------------------------------------------
// Low Level function
void AVC_ReleaseLine()
{
 AVC_SET_0();
 AVC_OUT_DIS();
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
void AVCLan_Init()
{
 // AVCLan TX+/TX- 	 : internal comparator PINB2, PINB3
 
  
 // OUTPUT ( set as input for comparator )
 cbi(PORTD, 6);
 cbi(DDRD, 6);

 // INPUT
 cbi(PORTD, 7);
 cbi(DDRD, 7);

 // Analog comparator
 
 cbi(ADCSRB, ACME);	// Analog Comparator Multiplexer Enable - NO
 cbi(ACSR, ACIS1);	// Analog Comparator Interrupt Mode Select
 cbi(ACSR, ACIS0);	// Comparator Interrupt on Output Toggle

 cbi(ACSR, ACD);	// Analog Comparator Disbale - NO


 message_len   = 0;
 answerReq     = cmNull;
 check_timeout = 0;

 cd_Disc = 0;
 cd_Track = 0;
 cd_Time_Min = 0;
 cd_Time_Sec = 0;
 repeatMode = 0;
 randomMode = 0;
 playMode   = 0;
 CD_Mode    = stStop;

}
//------------------------------------------------------------------------------
// Low Level function
uint8_t AVCLan_Read_Byte(uint8_t length)
{
 uint8_t byte = 0;
 uint8_t wT;
 
 while (1) {
   while (INPUT_IS_CLEAR);
   timer0_start();
   while (INPUT_IS_SET);
   wT = TCNT0;
   if (wT<8) { 
      byte++;
	  parity_bit++;
   }
   length--;
   if (!length) return byte;
   byte = byte << 1;
 } 
}
//------------------------------------------------------------------------------

uint8_t AVCLan_Send_StartBit()
{
 AVC_SET_1();
 //measured about 166us target is 166
 _delay_us(166);
 
 AVC_SET_0();
 //with value 30 measured 35us decreased to 25 target is 30
 _delay_us(25);

 return 1;
}
//------------------------------------------------------------------------------
void AVCLan_Send_Bit1()
{
 AVC_SET_1();
 //measured about 20us target is 20
 _delay_us(20);

 AVC_SET_0();
 //measured about 21us target is 20
 _delay_us(16);							// 12-21
}
//------------------------------------------------------------------------------
void AVCLan_Send_Bit0()
{
 AVC_SET_1();
 //measured about 32us target is 32
 _delay_us(32);							// 28-37
	
 AVC_SET_0();
 //with value 4 measured 5.3us increase to 5 target is 7
 _delay_us(5);								// 00-09
}
//------------------------------------------------------------------------------
uint8_t AVCLan_Read_ACK()
{
 uint8_t time = 0;

 AVC_SET_1();
 _delay_us(19);

 AVC_SET_0();
 _delay_us(1);


 AVC_OUT_DIS(); // switch to read mode
 timer0_source(CK64);
 timer0_start();
 while(1) {
	time = TCNT0;
	if (INPUT_IS_SET && (time > 1)) break;
	if (time > 20) return 1;
 }
	
 while(INPUT_IS_SET);
 AVC_OUT_EN();// back to write mode
 return 0;

}
//------------------------------------------------------------------------------
uint8_t AVCLan_Send_ACK()
{	
 timer0_source(CK64);						//update every 1us
 timer0_start();	
 while (INPUT_IS_CLEAR)	{
 	if (TCNT0 >= 25) return 0;			// max wait time
 }

 AVC_OUT_EN();

 AVC_SET_1();
 //like sending "0"
 _delay_us(32);								//28-37

 AVC_SET_0();
 _delay_us(5);									//00-09

 AVC_OUT_DIS();

 return 1;		
}
//------------------------------------------------------------------------------
uint8_t AVCLan_Send_Byte(uint8_t byte, uint8_t len)
{
 uint8_t b;
 if (len==8) {
 	b = byte;
 } else {
    b = byte << (8-len);
 }

 while (1) {
   if ( (b & 128)!=0 ) {
     AVCLan_Send_Bit1();
	 parity_bit++;
   } else { 
   	 AVCLan_Send_Bit0();
   }
   len--;
   if (!len) { 
     //if (INPUT_IS_SET) RS232_Print("SBER\n"); // Send Bit ERror
     return 1;
   }
   b = b << 1;
 } 

}
//------------------------------------------------------------------------------
uint8_t AVCLan_Send_ParityBit()
{
 if ( (parity_bit & 1)!=0 ) {
     AVCLan_Send_Bit1();
	 //parity_bit++;
 } else {
   	 AVCLan_Send_Bit0();
 }
 parity_bit=0;
 return 1;
}
//------------------------------------------------------------------------------
uint8_t CheckCmd(uint8_t *cmd)
{
 uint8_t i;
 uint8_t *c;
 uint8_t l;

 c = cmd;
 l = *c++;

 for (i=0; i<l; i++) {
 	if (message[i] != *c) return 0;
	c++;
 }
 return 1;
}
//------------------------------------------------------------------------------
uint8_t AVCLan_Read_Message()
{
 STOPEvent;						// disable timer1 interrupt

 uint8_t T = 0;

 uint8_t i;
 uint8_t for_me = 0;

 //RS232_Print("$ ");
 timer0_source(CK64);

 // check start bit
 timer0_start();
 while (INPUT_IS_SET) { 
 	T=TCNT0;
	if (T>254) {
		STARTEvent;
		RS232_Print("LAN>T1\n");
		return 0;
	}
 }


 if (T<10) {		// !!!!!!! 20 !!!!!!!!!!!
 	STARTEvent;
	RS232_Print("LAN>T2\n");
	return 0;
 }



 broadcast = AVCLan_Read_Byte(1);

 parity_bit = 0;
 master1 = AVCLan_Read_Byte(4);
 master2 = AVCLan_Read_Byte(8);
 if ((parity_bit&1)!=AVCLan_Read_Byte(1)) {
	STARTEvent;
	return 0;
 }

 parity_bit = 0;
 slave1 = AVCLan_Read_Byte(4);
 slave2 = AVCLan_Read_Byte(8);
 if ((parity_bit&1)!=AVCLan_Read_Byte(1)) {
	STARTEvent;
	return 0;
 }
 // is this command for me ?
 if ((slave1==CD_ID_1)&&(slave2==CD_ID_2)) {
 	for_me=1;
 }

 if (for_me) AVCLan_Send_ACK();
 		else AVCLan_Read_Byte(1);

 parity_bit = 0;
 AVCLan_Read_Byte(4);	// control - always 0xF
 if ((parity_bit&1)!=AVCLan_Read_Byte(1)) {
	STARTEvent;
	return 0;
 }
 if (for_me) AVCLan_Send_ACK();
 		else AVCLan_Read_Byte(1);

 parity_bit = 0;
 message_len = AVCLan_Read_Byte(8);
 if ((parity_bit&1)!=AVCLan_Read_Byte(1)) {
	STARTEvent;
	return 0;
 }
 if (for_me) AVCLan_Send_ACK();
 		else AVCLan_Read_Byte(1);

 if (message_len > MAXMSGLEN) {
//	RS232_Print("LAN> Command error");
	STARTEvent;
	return 0;
 }

 for (i=0; i<message_len; i++) {
	parity_bit = 0;
 	message[i] = AVCLan_Read_Byte(8);
	if ((parity_bit&1)!=AVCLan_Read_Byte(1)) {
		STARTEvent;
		return 0;
 	}
	if (for_me) {
		AVCLan_Send_ACK();
 	} else {
		AVCLan_Read_Byte(1);
	}
 }


 STARTEvent;

 if (logLevel>0) ShowInMessage();

 if (for_me) {
 	
	if (CheckCmd((uint8_t*)stat1)) { answerReq = cmStatus1; return 1; }
	if (CheckCmd((uint8_t*)stat2)) { answerReq = cmStatus2; return 1; }
	if (CheckCmd((uint8_t*)stat3)) { answerReq = cmStatus3; return 1; }
	if (CheckCmd((uint8_t*)stat4)) { answerReq = cmStatus4; return 1; }

	if (CheckCmd((uint8_t*)play_req1)) { answerReq = cmPlayReq1; return 1; }
	if (CheckCmd((uint8_t*)play_req2)) { answerReq = cmPlayReq2; return 1; }
	if (CheckCmd((uint8_t*)play_req3)) { answerReq = cmPlayReq3; return 1; }
	if (CheckCmd((uint8_t*)stop_req))  { answerReq = cmStopReq;  return 1; }
	if (CheckCmd((uint8_t*)stop_req2)) { answerReq = cmStopReq2; return 1; }
        if (CheckCmd((uint8_t*)btn_scan))  { answerReq = cmNull; RS232_Print("Btn Scan\n"); return 1; }

 } else { // broadcast check

	if (CheckCmd((uint8_t*)lan_playit))	{ answerReq = cmPlayIt;	return 1; }
	if (CheckCmd((uint8_t*)lan_check))	{ 
			answerReq = cmCheck;
			CMD_CHECK[6]=message[3];
			return 1; 
	}
	if (CheckCmd((uint8_t*)lan_reg))	{ answerReq = cmRegister;	return 1; }
	if (CheckCmd((uint8_t*)lan_init))	{ answerReq = cmInit;		return 1; }
	if (CheckCmd((uint8_t*)lan_stat1))	{ answerReq = cmStatus1;	return 1; }


 }
 answerReq = cmNull;
 return 1;
}
//------------------------------------------------------------------------------
uint8_t AVCLan_SendData()
{
 uint8_t i;

 STOPEvent;

 // wait for free line
 uint8_t T=0;
 uint8_t line_busy = 1;

 timer0_source(CK64);
 timer0_start();
 do {
 	while (INPUT_IS_CLEAR) {
		T = TCNT0;
		if (T >= 25) break;
 	}
 	if (T > 24) line_busy=0;
 } while (line_busy);


 // switch to output mode
 AVC_OUT_EN();

 AVCLan_Send_StartBit();
 AVCLan_Send_Byte(0x1,  1);	// regular communication


 parity_bit = 0;
 AVCLan_Send_Byte(CD_ID_1, 4);	// CD Changer ID as master
 AVCLan_Send_Byte(CD_ID_2, 8);
 AVCLan_Send_ParityBit();

 AVCLan_Send_Byte(HU_ID_1, 4);	// HeadUnit ID as slave
 AVCLan_Send_Byte(HU_ID_2, 8);

 AVCLan_Send_ParityBit();

 if (AVCLan_Read_ACK()) {
 	 AVC_OUT_DIS();
	 STARTEvent;
	 RS232_Print("E1\n");
	 return 1;
 }


 AVCLan_Send_Byte(0xF, 4);	// 0xf - control -> COMMAND WRITE
 AVCLan_Send_ParityBit();
 if (AVCLan_Read_ACK()) {
 	 AVC_OUT_DIS();
	 STARTEvent;
	 RS232_Print("E2\n");
	 return 2;
 }

 AVCLan_Send_Byte(data_len,  8);// data lenght
 AVCLan_Send_ParityBit();
 if (AVCLan_Read_ACK()) {
 	 AVC_OUT_DIS();
	 STARTEvent;
	 RS232_Print("E3\n");
	 return 3;
 }

 for (i=0;i<data_len;i++) {
	AVCLan_Send_Byte(data[i], 8);// data byte
 	AVCLan_Send_ParityBit();
 	if (AVCLan_Read_ACK()) {
	 	 AVC_OUT_DIS();
		 STARTEvent;
 		 RS232_Print("E4(");
		 RS232_PrintDec(i);
		 RS232_Print(")\n");
		 return 4;
 	}
 }

 // back to read mode
 AVC_OUT_DIS();

 STARTEvent;
 if (logLevel>0) ShowOutMessage();
 return 0;
}
//------------------------------------------------------------------------------
uint8_t AVCLan_SendDataBroadcast()
{
 uint8_t i;

 STOPEvent;

 // wait for free line
 uint8_t T=0;
 uint8_t line_busy = 1;

 timer0_source(CK64);
 timer0_start();
 do {
 	while (INPUT_IS_CLEAR) {
		T = TCNT0;
		if (T >= 25) break;
 	}
 	if (T > 24) line_busy=0;
 } while (line_busy);


 AVC_OUT_EN();

 AVCLan_Send_StartBit();
 AVCLan_Send_Byte(0x0,  1);		// broadcast

 parity_bit = 0;
 AVCLan_Send_Byte(CD_ID_1, 4);		// CD Changer ID as master
 AVCLan_Send_Byte(CD_ID_2, 8);
 AVCLan_Send_ParityBit();

 AVCLan_Send_Byte(0x1, 4);		// all audio devices
 AVCLan_Send_Byte(0xFF, 8);
 AVCLan_Send_ParityBit();
 AVCLan_Send_Bit1();

 AVCLan_Send_Byte(0xF, 4);		// 0xf - control -> COMMAND WRITE
 AVCLan_Send_ParityBit();
 AVCLan_Send_Bit1();
 
 AVCLan_Send_Byte(data_len,  8);	// data lenght
 AVCLan_Send_ParityBit();
 AVCLan_Send_Bit1();

 for (i=0;i<data_len;i++) {
	AVCLan_Send_Byte(data[i], 8); 	// data byte
 	AVCLan_Send_ParityBit();
	AVCLan_Send_Bit1();
 }

 AVC_OUT_DIS();
 STARTEvent;
 if (logLevel>0) ShowOutMessage();
 return 0;
}
//------------------------------------------------------------------------------
uint8_t AVCLan_SendAnswerFrame(uint8_t *cmd)
{
 uint8_t i;
 uint8_t *c;
 uint8_t b;

 c = cmd;
 
 b = *c++;
 data_control = 0xF;
 data_len	 = *c++;
 for (i=0; i<data_len; i++) {
 	data[i]= *c++;
 }
 if (b)
 	return AVCLan_SendData();
 else 
 	return AVCLan_SendDataBroadcast();
}
//------------------------------------------------------------------------------
uint8_t AVCLan_SendMyData(uint8_t *data_tmp, uint8_t s_len)
{
 uint8_t i;
 uint8_t *c;
 
 c = data_tmp;
 
 data_control = 0xF;
 data_len	 = s_len;
 for (i=0; i<data_len; i++) {
 	data[i]= *c++;
 }
 return AVCLan_SendData();
}
//------------------------------------------------------------------------------
uint8_t AVCLan_SendMyDataBroadcast(uint8_t *data_tmp, uint8_t s_len)
{
 uint8_t i;
 uint8_t *c;
 

 c = data_tmp;
 
 data_control = 0xF;
 data_len	 = s_len;
 for (i=0; i<data_len; i++) {
 	data[i]= *c++;
 }
 return AVCLan_SendDataBroadcast();
}
//------------------------------------------------------------------------------
uint8_t AVCLan_SendInitCommands()
{
 uint8_t r;

 const uint8_t c1[] = { 0x0, 0x0B, 0x63, 0x31, 0xF1, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x80 };
 const uint8_t c2[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x02 };
 const uint8_t c3[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x3F, 0x00, 0x01, 0x00, 0x01, 0x02 };
 const uint8_t c4[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x3D, 0x00, 0x01, 0x00, 0x01, 0x02 };
 const uint8_t c5[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x39, 0x00, 0x01, 0x00, 0x01, 0x02 };
 const uint8_t c6[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x31, 0x00, 0x01, 0x00, 0x01, 0x02 };
 const uint8_t c7[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x21, 0x00, 0x01, 0x00, 0x01, 0x02 };
 const uint8_t c8[] = { 0x0, 0x0B, 0x63, 0x31, 0xF1, 0x00, 0x90, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0x80 };
 const uint8_t c9[] = { 0x0, 0x0A, 0x63, 0x31, 0xF3, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x02 };
 const uint8_t cA[] = { 0x0, 0x0B, 0x63, 0x31, 0xF1, 0x00, 0x30, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0x80 };

 r = AVCLan_SendAnswerFrame((uint8_t*)c1);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c2);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c3);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c4);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c5);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c6);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c7);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c8);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)c9);
 if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)cA);

 return r;
}
//------------------------------------------------------------------------------
void AVCLan_Send_Status()
{
//                                                        disc  track t_min t_sec
 uint8_t STATUS[] = {0x0, 0x0B, 0x63, 0x31, 0xF1, 0x01, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, 0x80 };
	
 STATUS[7] =  cd_Disc;
 STATUS[8] =  cd_Track;
 STATUS[9] =  cd_Time_Min;
 STATUS[10] = cd_Time_Sec;

 STATUS[11] = 0;

 AVCLan_SendAnswerFrame((uint8_t*)STATUS);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
uint8_t AVCLan_SendAnswer()
{
 uint8_t r = 0 ;
 
 switch (answerReq) {
 	case cmStatus1:		r = AVCLan_SendAnswerFrame((uint8_t*)CMD_STATUS1); 
						break;
 	case cmStatus2:		r = AVCLan_SendAnswerFrame((uint8_t*)CMD_STATUS2); 
						break;
 	case cmStatus3:		r = AVCLan_SendAnswerFrame((uint8_t*)CMD_STATUS3); 
						break;
 	case cmStatus4:		r = AVCLan_SendAnswerFrame((uint8_t*)CMD_STATUS4); 
						break;
 	case cmRegister:	r = AVCLan_SendAnswerFrame((uint8_t*)CMD_REGISTER); 
						break;
 	case cmInit:
						r = AVCLan_SendInitCommands(); 
						break;
 	case cmCheck:		r = AVCLan_SendAnswerFrame((uint8_t*)CMD_CHECK); 
						check_timeout = 0;
						CMD_CHECK[6]++;
 						RS232_Print("AVCCHK\n");
						break;
 	case cmPlayReq1:	playMode = 0;
						r = AVCLan_SendAnswerFrame((uint8_t*)CMD_PLAY_OK1); 
						break;
 	case cmPlayReq2:	
	case cmPlayReq3:	playMode = 0;
						r = AVCLan_SendAnswerFrame((uint8_t*)CMD_PLAY_OK2); 
						if (!r) r = AVCLan_SendAnswerFrame((uint8_t*)CMD_PLAY_OK3);
						CD_Mode = stPlay;
						break;
	case cmPlayIt:		playMode = 1;
						RS232_Print("PLAY\n");
						CMD_PLAY_OK4[7]=cd_Disc;
						CMD_PLAY_OK4[8]=cd_Track;
						CMD_PLAY_OK4[9]=cd_Time_Min;
						CMD_PLAY_OK4[10]=cd_Time_Sec;
						r = AVCLan_SendAnswerFrame((uint8_t*)CMD_PLAY_OK4); 
						if (!r) AVCLan_Send_Status();
						CD_Mode = stPlay;
						break;
	case cmStopReq:
	case cmStopReq2:	CD_Mode = stStop;
						playMode = 0;
						
						r = AVCLan_SendAnswerFrame((uint8_t*)CMD_STOP1); 
						CMD_STOP2[7]=cd_Disc;
						CMD_STOP2[8]=cd_Track;
						CMD_STOP2[9]=cd_Time_Min;
						CMD_STOP2[10]=cd_Time_Sec;
						r = AVCLan_SendAnswerFrame((uint8_t*)CMD_STOP2); 
						break;
	case cmBeep:		AVCLan_SendAnswerFrame((uint8_t*)CMD_BEEP);
						break;
 }

 answerReq = cmNull;
 return r;
}
//------------------------------------------------------------------------------
void AVCLan_Register()
{
 RS232_Print("REG_ST\n");
 AVCLan_SendAnswerFrame((uint8_t*)CMD_REGISTER); 
 RS232_Print("REG_END\n");
 //AVCLan_Command( cmRegister );
 AVCLan_Command( cmInit );
}
//------------------------------------------------------------------------------
uint8_t	 AVCLan_Command(uint8_t command)
{
 uint8_t r;

 answerReq = command;
 r = AVCLan_SendAnswer(); 
 /*
 RS232_Print("ret=");
 RS232_PrintHex8(r);
 RS232_Print("\n");
 */
 return r;
}
//------------------------------------------------------------------------------
uint8_t HexInc(uint8_t data)
{
 if ((data & 0x9)==0x9) 
 	return (data + 7);
 
 return (data+1);
}
//------------------------------------------------------------------------------
uint8_t HexDec(uint8_t data)
{
 if ((data & 0xF)==0) 
 	return (data - 7);
 
 return (data-1);
}
//------------------------------------------------------------------------------
// encode decimal valute to 'toyota' format :-)
//  ex.   42 (dec)   =  0x42 (toy)
uint8_t Dec2Toy(uint8_t data)
{
 uint8_t d,d1;
 d = (uint32_t)data/(uint32_t)10;
 d1 = d * 16;
 d  = d1 + (data - 10*d);
 return d;
}
//------------------------------------------------------------------------------
void ShowInMessage()
{
 if (message_len==0) return;

 AVC_HoldLine();
 

 RS232_S((uintptr_t)PSTR("HU < ("));

 if (broadcast==0) RS232_S((uintptr_t)PSTR("bro) "));
 else RS232_Print("dir) ");

 RS232_PrintHex4(master1);
 RS232_PrintHex8(master2);
 RS232_Print("| ");
 RS232_PrintHex4(slave1);
 RS232_PrintHex8(slave2);
 RS232_Print("| ");
 
 uint8_t i;
 for (i=0;i<message_len;i++) {
	RS232_PrintHex8(message[i]);
	RS232_Print(" ");
 }
 RS232_Print("\n");

 AVC_ReleaseLine();
}
//------------------------------------------------------------------------------
void ShowOutMessage()
{
 uint8_t i;

 AVC_HoldLine();
 
 RS232_S((uintptr_t)PSTR("out > "));
 for (i=0; i<data_len; i++) {
	RS232_PrintHex8(data[i]);
	RS232_SendByte(' ');
 }
 RS232_Print("\n");

 AVC_ReleaseLine();
}

//------------------------------------------------------------------------------
