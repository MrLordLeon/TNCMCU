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
int AX25_temp_buffer[max_packet_count]; //temperary stores bits received from radio, before formatting into AX.25 format
int packet_count = 0; //keeps count of the temp buffer index
//*********************************************************************************************************************

//*************** AX.25 Fields ******************************************************************************************
int start_flag[flag_len];
int address[address_len];
int control[control_len];
int PID[PID_len]; //only for information subfield
int Info[Info_len]; //only for information subfield
int FCS[FCS_len];
int end_flag[flag_len];
//***********************************************************************************************************************


//************* Handle bits received from Radio *************************************************************************
//while the AX25_flag is true, start storing bits in temp buffer
void store_radio_bits(){
	int bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0;

	//keep checking for ending flag
	while(AX25_flag && packet_count < max_packet_count && !changeMode){
		//Slide bits
		bit7 = bit6;
		bit6 = bit5;
		bit5 = bit4;
		bit4 = bit3;
		bit3 = bit2;
		bit2 = bit1;
		bit1 = bit0;
		bit0 = loadBit();
		AX25_temp_buffer[packet_count++] = bit0; //store bit into AX25 buffer

		//Detect 0x7
		if((bit7==0)&&(bit6==1)&&(bit5==1)&&(bit4==1)){
			//Detect 0x7E
			if((bit3==1)&&(bit2==1)&&(bit1==1)&&(bit0==0)){
				AX25_flag = !AX25_flag;
				packet_count = 0;
				sprintf(uartData, "AX.25 Flag Detected\r\n");
			}
		}
		else {
			sprintf(uartData, "Bits detected: %d %d %d %d %d %d %d %d\r\n",bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0);
		}

		HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	}
}

//check if we keep the packet or remove it
bool Packet_Validate(){
	bool is_valid;
	if(packet_count < 120){ //invalid if packet is less than 136 bits - 2*8 bits (per flag)
		sprintf(uartData,"Trash Packet");
		is_valid = false;
	}
	else if((packet_count+1)%8 != 0){ //invalid if packet is not octect aligned (divisible by 8)
		sprintf(uartData,"Trash Packet");
		is_valid = false;
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
		is_valid = true;
	}

	//reset buffer and bit counter
	packet_count = 0;
	memset(AX25_temp_buffer,0,max_packet_count);
	HAL_UART_Transmit(&huart2, uartData, strlen(uartData), 10);
	return is_valid;
}
