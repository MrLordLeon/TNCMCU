/*
 * AX.25.c
 *
 *  Created on: Sep 2, 2020
 *      Author: Kobe
 */
#include "AX.25.h"
#include "FreqIO.h"

//*************** variables for detecting and validating  AX.25  ******************************************************
bool AX25_flag = false; //indicates whether the TNC started reading for packets
extern int AX25_temp_buffer[max_packet_count]; //temperary stores bits received from radio, before formatting into AX.25 format
int packet_count = 0; //keeps count of the temp buffer index
//*********************************************************************************************************************

//*************** AX.25 Fields ******************************************************************************************
int address[address_len];
int control[control_len];
int PID[PID_len]; //only for information subfield
int Info[Info_len]; //only for information subfield
int FCS[FCS_len];
//***********************************************************************************************************************


//************* Handle bits received from Radio *************************************************************************
//while the AX25_flag is true, start storing bits in temp buffer
void store_radio_bits(int bit){
	AX25_temp_buffer[packet_count] = bit;
	if(packet_count < max_packet_count){
		packet_count++;
	}
	else{
		packet_count = 0;
	}
}

//check if we keep the packet or remove it
bool Packet_Validate(){
	if(packet_count < 120){ //invalid if packet is less than 136 bits - 2*8 bits (per flag)
		sprintf(uartData,"Trash Packet");
	}
	else if((packet_count+1)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
		sprintf(uartData,"Trash Packet");
	}
	else{
		//this is assuming that the packet has all the subfields full
		sprintf(uartData,"Good Packet!");
		int *curr_mem = &AX25_temp_buffer; //keep track of what address to copy from
		memcpy(address,curr_mem,address_len*sizeof(int));
		curr_mem += address_len;
		memcpy(control,curr_mem,control_len*sizeof(int));
		curr_mem += control_len;
		if(packet_count > max_packet_count - PID_len){ //information type packet
			memcpy(PID,curr_mem,PID_len*sizeof(int));
			curr_mem += PID_len;
		}
		memcpy(Info,curr_mem,Info_len*sizeof(int));
		curr_mem += Info_len;
		memcpy(FCS,curr_mem,FCS_len*sizeof(int));
	}

	//reset buffer and bit counter
	packet_count = 0;
	memset(AX25_temp_buffer,0,3000);

}
