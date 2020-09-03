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
extern bool AX25_flag; //indicates whether the TNC started reading for packets
extern int AX25_temp_buffer[3000]; //temperary stores bits received from radio, before formatting into AX.25 format
//*********************************************************************************************************************

//*************** AX.25 Field lengths ********************************************************************
#define address_len		560 //max number of bits in address field
#define control_len		8 //number of bits in control field
#define PID_len			8 //number of bits in PID field (subfield for info frames)
#define Info_len		2048 //Max number of bits in information field(sub field for info frames)
#define FCS_len			16 //number of bits in FCS frames
//********************************************************************************************************

//*************** AX.25 Formatt ************************************************************************
typedef struct AX_25{
	int address[address_len];
	int control[control_len];
	int PID[PID_len]; //only for information subfield
	int INFO[Info_len]; //only for information subfield
	int FCS[FCS_len];
} AX_25;

//************* Handle bits received from Radio *************************************************************************
void store_radio_bits(int bit); //while the AX25_flag is true, start storing bits in temp buffer


#endif /* SRC_AX_25_H_ */
