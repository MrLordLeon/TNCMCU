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

//*************** variables for detecting and validating  AX.25  ******************************************************
#define max_packet_count		2320//max bits in a packet, not including flags

extern bool AX25_flag; //indicates whether the TNC started reading for packets
extern int AX25_temp_buffer[max_packet_count]; //temperary stores bits received from radio, before formatting into AX.25 format
extern int packet_count; //keeps count of the temp buffer index
//*********************************************************************************************************************

//*************** AX.25 Fields********************************************************************
#define address_len		224 //min number of bits in address field
#define control_len		8 //number of bits in control field
#define PID_len			8 //number of bits in PID field (subfield for info frames)
#define Info_len		2048 //Max number of bits in information field(sub field for info frames)
#define FCS_len			16 //number of bits in FCS frames
#define flag_len		8 //number of bits in flag

extern int start_flag[flag_len];
extern int address[address_len];
extern int control[control_len];
extern int PID[PID_len]; //only for information type packet
extern int Info[Info_len];
extern int FCS[FCS_len];
extern int end_flag[flag_len];
//********************************************************************************************************


//************* Handle bits received from Radio *************************************************************************
void store_radio_bits(); //while the AX25_flag is true, start storing bits in temp buffer
bool Packet_Validate();

#endif /* SRC_AX_25_H_ */
