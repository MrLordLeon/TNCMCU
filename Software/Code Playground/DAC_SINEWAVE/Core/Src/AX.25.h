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
#define address_len		112 //min number of bits in address field/224 bits is the max for digipeting
#define control_len		8 //MIN number of bits in control field/MAX is 16 bits supposedly
#define PID_len			8 //number of bits in PID field (subfield for info frames)
#define MAX_INFO		2048 //Max number of bits in information field(sub field for info frames)
#define FCS_len			16 //number of bits in FCS frames
#define MAX_Stuffed     463 //possible max number of bit stuffed zeros
#define bool_size		sizeof(bool) //size of a boolean data type, in case you didn't know

//*************** variables for detecting and validating  AX.25  ******************************************************
#define AX25_PACKET_MAX		address_len + control_len + PID_len + MAX_INFO +FCS_len	+ MAX_Stuffed		//max bits in a packet, not including flags

extern int rxBit_count; 							//keeps count of the temp buffer index
extern bool AX25TBYTE[FLAG_SIZE];							//Array to store AX.25 terminate flag in binary
extern bool local_address[address_len/2];	//address set to this TNC
//*********************************************************************************************************************

//**************** KISS *************************************************************************************************************
#define KISS_SIZE		FLAG_SIZE + address_len + control_len + PID_len + MAX_INFO + FLAG_SIZE //size of kiss packet
bool KISS_FLAG[FLAG_SIZE];
void generate_KISS();
void remove_bit_stuffing(); //remove bit stuffing zeros after every 5 consecutive 1's
void slide_bits(bool* array,int bits_left); //discards bit stuffed 0 and slide remaining bits over bits over
//*************************************************************************************************************************************

//General Program
//****************************************************************************************************************
struct PACKET_STRUCT {
	//AX.25 Members
	bool AX25_temp_buffer[AX25_PACKET_MAX];//temporary stores bits received from radio, before formatting into AX.25 format
	bool *address;			//Pointer to address field in global buffer
	bool *control;			//Pointer to control field in global buffer
	bool *PID; 				//Pointer to PID field in global buffer, only present for I frames
	bool *Info;				//Pointer to info field in global buffer
	int  Info_Len;
	bool *FCS;				//Pointer to fcs field in global buffer

	//KISS Members
	bool KISS_PACKET[KISS_SIZE];

	//CRC
	int crc; 				//crc value after calculating data from PC
}global_packet;

void tx_rx();
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
void receiving_AX25();
void transmitting_AX25();
void ouput_AX25();
#endif /* SRC_AX_25_H_ */
