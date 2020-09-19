/*
 * AX.25.h
 *
 *  Created on: Sep 2, 2020
 *      Author: Kobe
 *
 *      This file is for formatting and detecting AX.25 Packets
 */

#ifndef SRC_AX_25_H_
#define SRC_AX_25_H_
#include "main.h"
#include "FreqIO.h"
#include <stdio.h>
#include <stdbool.h>

//*************** AX.25 Fields******************************************************************************************
#define FLAG_SIZE		8
#define address_len		112 //min number of bits in address field/224 bits is the max
#define control_len		8 //MIN number of bits in control field/MAX is 16 bits
#define PID_len			8 //number of bits in PID field (subfield for info frames)
#define Info_len		2048 //Max number of bits in information field(sub field for info frames)
#define FCS_len			16 //number of bits in FCS frames
#define bool_size		sizeof(bool) //size of a boolean data type, in case you didn't know

extern bool address[address_len];
extern bool control[control_len];
extern bool PID[PID_len]; //only for information type packet
extern bool Info[Info_len];
extern bool FCS[FCS_len];

extern int crc; //crc value after calculating data from PC
extern bool end_flag; 		//flag to indicate the end of a KISS Packet
//********************************************************************************************************

//*************** variables for detecting and validating  AX.25  ******************************************************
#define AX25_PACKET_MAX		2192					//max bits in a packet, not including flags

extern bool AX25_flag; 								//indicates whether the TNC started reading for packets
extern bool AX25_temp_buffer[AX25_PACKET_MAX]; 		//temporary stores bits received from radio, before formatting into AX.25 format
extern int packet_count; 							//keeps count of the temp buffer index
extern bool AX25TBYTE[FLAG_SIZE];							//Array to store AX.25 terminate flag in binary
extern bool buffer_rdy;								//flag indicates when closing flag has been sent
extern bool local_address[address_len/2];							//address set to this TNC
//*********************************************************************************************************************

//**************** KISS *************************************************************************************************************
#define KISS_SIZE		FLAG_SIZE + address_len + control_len + PID_len + Info_len + FLAG_SIZE //size of kiss packet
bool KISS_FLAG[FLAG_SIZE];
bool KISS_PACKET[KISS_SIZE];
void generate_KISS();
void remove_bit_stuffing(); //remove bit stuffing zeros after every 5 consecutive 1's
void slide_bits(bool* array,int bits_left); //discards bit stuffed 0 and slide remaining bits over bits over
//*************************************************************************************************************************************

//General Program
//****************************************************************************************************************

//************* Handle bits received from Radio *************************************************************************
/*
 * 	Function that simply loads a bit into the packet. Should be called
 * 	only after a start flag is detected
 */
void packetBit();
void loadPacket(); //while the AX25_flag is true, start storing bits in temp buffer
bool Packet_Validate();
void generate_address();
bool compare_address();
void crc_calc(int in_bit);
void hex_to_bin();						//store bits in FCS field
#endif /* SRC_AX_25_H_ */
